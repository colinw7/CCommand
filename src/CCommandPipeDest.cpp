#include "CCommandI.h"
#include <cerrno>
#include <cstring>

CCommandPipeDest::
CCommandPipeDest(CCommand *command) :
 CCommandDest(command)
{
  pipe_src_ = NULL;
  pipe_     = NULL;
}

CCommandPipeDest::
~CCommandPipeDest()
{
  term();

  if (pipe_src_ != NULL)
    pipe_src_->setDest(NULL);
}

void
CCommandPipeDest::
setSrc(CCommandPipeSrc *pipe_src)
{
  pipe_src_ = pipe_src;
}

void
CCommandPipeDest::
setPipe(CCommandPipe *pipe)
{
  pipe_ = pipe;

  if (pipe_ != NULL)
    pipe_->setDest(command_);
}

void
CCommandPipeDest::
addFd(int fd)
{
  dest_fds_.push_back(fd);
}

void
CCommandPipeDest::
initParent()
{
  pipe_ = new CCommandPipe(command_);

  pipe_->setDest(command_);

  if (pipe_src_ != NULL)
    pipe_src_->setPipe(pipe_);
}

void
CCommandPipeDest::
initChild()
{
  bool close_output = true;

  // Pipe Destination Command writes output of command to output of pipe
  if (command_->getDoFork()) {
    // close child command destination files (stdout and/or stderr) and
    // redirect pipe output to destination files (stdout and/or stderr)
    for (uint i = 0; i < dest_fds_.size(); ++i) {
      int error = ::close(dest_fds_[i]);
      if (error < 0) throwError(string("close: ") + strerror(errno));

      if (pipe_->getOutput() != dest_fds_[i]) {
        error = ::dup2(pipe_->getOutput(), dest_fds_[i]);
        if (error < 0) throwError(string("dup2: ") + strerror(errno));
      }
      else
        close_output = false;
    }

    // close pipe input (not needed after fork)
    int error = pipe_->closeInput();
    if (error < 0) throwError(string("close: ") + strerror(errno));
  }
  else {
    // save command destination files (stdout and/or stderr) and
    // redirect pipe output to destination files (stdout and/or stderr)
    for (uint i = 0; i < dest_fds_.size(); ++i) {
      int save_fd = ::dup(dest_fds_[i]);
      if (save_fd < 0) throwError(string("dup: ") + strerror(errno));

      save_fds_.push_back(save_fd);

      if (pipe_->getOutput() != dest_fds_[i]) {
        int error = ::dup2(pipe_->getOutput(), dest_fds_[i]);
        if (error < 0) throwError(string("dup2: ") + strerror(errno));
      }
      else
        close_output = false;
    }

    // don't close pipe input (used in same process)
  }

  // close pipe output (already redirected)
  if (close_output) {
    int error = pipe_->closeOutput();
    if (error < 0) throwError(string("close: ") + strerror(errno));
  }
}

void
CCommandPipeDest::
term()
{
  if (pipe_ != NULL) {
    if (command_->getDoFork()) {
      // close pipe input (not needed after fork)
      int error = pipe_->closeInput();
      if (error < 0) throwError(string("close: ") + strerror(errno));
    }

    // close pipe output (already redirected)
    int error = pipe_->closeOutput();
    if (error < 0) throwError(string("close: ") + strerror(errno));
  }

  // restore redirected destination files (stdout and/or stderr)
  for (int i = 0; i < (int) save_fds_.size(); i++) {
    ::dup2(save_fds_[i], dest_fds_[i]);

    ::close(save_fds_[i]);
  }

  save_fds_.clear();
}
