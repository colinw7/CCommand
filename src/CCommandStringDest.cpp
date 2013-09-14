#include "CCommandI.h"
#include <COSFile.h>
#include <cstdio>
#include <cerrno>
#include <cstring>

//#define USE_PIPE 1

CCommandStringDest::
CCommandStringDest(CCommand *command, string &str, int dest_fd) :
 CCommandDest(command), str_(str), dest_fd_(dest_fd)
{
  pipe_ = NULL;
  fd_   = 0;
}

CCommandStringDest::
~CCommandStringDest()
{
  term();

  delete pipe_;
}

void
CCommandStringDest::
initParent()
{
#ifdef USE_PIPE
  pipe_ = new CCommandPipe(command_);

  save_fd_ = dup(dest_fd_);

  if (save_fd_ < 0)
    throwError(string("dup: ") + strerror(errno));

  int error = close(dest_fd_);

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  error = dup2(pipe_->getOutput(), dest_fd_);

  if (error < 0)
    throwError(string("dup2: ") + strerror(errno));
#else
  fd_ = COSFile::getTempFileNum(filename_, /*del_on_close*/ true);

  if (fd_ < 0)
    throwError("COSFile::getTempFileNum: failed");

  save_fd_ = dup(dest_fd_);

  if (save_fd_ < 0)
    throwError(string("dup: ") + strerror(errno));

  int error = close(dest_fd_);

  error = dup2(fd_, dest_fd_);

  if (error < 0)
    throwError(string("dup2: ") + strerror(errno));
#endif
}

void
CCommandStringDest::
initChild()
{
}

void
CCommandStringDest::
term()
{
#ifdef USE_PIPE
#else
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Term string dest\n");

  fsync(dest_fd_);

  if (save_fd_ != -1) {
    dup2(save_fd_, dest_fd_);

    close(save_fd_);

    save_fd_ = -1;
  }

  fsync(fd_);

#if 0
  int error = close(fd_);

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  fd_ = open(filename_.c_str(), O_RDONLY);

  if (fd_ == -1)
    throwError(string("open: ") + strerror(errno));
#endif

  int offset = lseek(fd_, 0, SEEK_SET);

  if (offset < 0)
    throwError(string("lseek: ") + strerror(errno));

  char buffer[1025];

  int num_read;

  while ((num_read = read(fd_, buffer, 1024)) > 0) {
    buffer[num_read] = 0;

    str_ += buffer;
  }

  int error = close(fd_);

  if (error < 0)
    throwError(string("close: ") + strerror(errno));
#endif
}

void
CCommandStringDest::
process()
{
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Process string dest\n");

#ifdef USE_PIPE
  if (save_fd_ != -1) {
    dup2(save_fd_, dest_fd_);

    close(save_fd_);

    save_fd_ = -1;
  }

  int error = pipe_->closeOutput();

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  fd_ = pipe_->getInput();

  if (fd_ != -1) {
    char buffer[1025];

    int num_read;

    while ((num_read = read(fd_, buffer, 1024)) > 0) {
      buffer[num_read] = 0;

      str_ += buffer;
    }

    int error = pipe_->closeInput();

    if (error < 0)
      throwError(string("close: ") + strerror(errno));
  }
#else
#endif
}
