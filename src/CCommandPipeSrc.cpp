#include <CCommandPipeSrc.h>
#include <CCommandPipeDest.h>
#include <CCommandPipe.h>
#include <CCommand.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <unistd.h>

// Note: pipe source owns the pipe used by piep source and destination

CCommandPipeSrc::
CCommandPipeSrc(CCommand *command) :
 CCommandSrc(command)
{
}

CCommandPipeSrc::
~CCommandPipeSrc()
{
  term();

  if (pipe_dest_)
    pipe_dest_->setSrc(nullptr);

  if (pipe_dest_)
    pipe_dest_->setPipe(nullptr);

  delete pipe_;
}

void
CCommandPipeSrc::
setDest(CCommandPipeDest *pipe_dest)
{
  pipe_dest_ = pipe_dest;
}

void
CCommandPipeSrc::
setPipe(CCommandPipe *pipe)
{
  pipe_ = pipe;

  if (pipe_)
    pipe_->setSrc(command_);
}

void
CCommandPipeSrc::
initParent()
{
}

void
CCommandPipeSrc::
initChild()
{
  // must have pipe set
  assert(pipe_);

  // Pipe Source Command reads input of command fron input of pipe
  if (command_->getDoFork()) {
    // close stdin
    int error = ::close(0);
    if (error < 0) throwError(std::string("close: ") + strerror(errno));

    if (pipe_->getInput() != 0) {
      // redirect pipe input to stdin
      error = ::dup2(pipe_->getInput(), 0);
      if (error < 0) throwError(std::string("dup2: ") + strerror(errno));

      // close pipe input
      error = pipe_->closeInput();
      if (error < 0) throwError(std::string("close: ") + strerror(errno));
    }

    // close pipe output (not needed)
    error = pipe_->closeOutput();
    if (error < 0) throwError(std::string("close: ") + strerror(errno));
  }
  else {
    // save orginal stdin
    save_stdin_ = dup(0);
    if (save_stdin_ < 0) throwError(std::string("dup: ") + strerror(errno));

    if (pipe_->getInput() != 0) {
      // redirect pipe input to stdin
      int error = dup2(pipe_->getInput(), 0);
      if (error < 0) throwError(std::string("dup2: ") + strerror(errno));

      // close pipe input
      error = pipe_->closeInput();
      if (error < 0) throwError(std::string("close: ") + strerror(errno));
    }
  }
}

void
CCommandPipeSrc::
term()
{
  if (pipe_) {
    // close pipe input (already redirected)
    int error = pipe_->closeInput();
    if (error < 0) throwError(std::string("close: ") + strerror(errno));

    if (command_->getDoFork()) {
      // close pipe output (not needed after fork)
      int error1 = pipe_->closeOutput();
      if (error1 < 0) throwError(std::string("close: ") + strerror(errno));
    }
  }

  // restore redirected source files (stdin)
  if (save_stdin_ != -1) {
    ::dup2(save_stdin_, 0);

    ::close(save_stdin_);
  }

  save_stdin_ = -1;
}
