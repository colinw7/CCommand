#include "CCommandI.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

CCommandFileDest::
CCommandFileDest(CCommand *command, const string &file, int dest_fd) :
 CCommandDest(command), dest_fd_(dest_fd)
{
  file_ = new string(file);

  overwrite_ = true;
  append_    = false;
}

CCommandFileDest::
CCommandFileDest(CCommand *command, FILE *fp, int dest_fd) :
 CCommandDest(command, fp), dest_fd_(dest_fd)
{
  file_ = NULL;

  overwrite_ = true;
  append_    = false;
}

CCommandFileDest::
~CCommandFileDest()
{
  delete file_;

  term();
}

void
CCommandFileDest::
initParent()
{
  if (file_ != NULL) {
    if (append_) {
      if (! overwrite_ && ! CFile::exists(*file_))
        throwError(*file_ + ": No such file or directory.");

      if (! CFile::exists(*file_))
        fd_ = creat(file_->c_str(), 0666);
      else
        fd_ = open(file_->c_str(), O_WRONLY | O_APPEND);
    }
    else {
      if (! overwrite_ && CFile::exists(*file_))
        throwError(*file_ + ": File exists.");

      fd_ = creat(file_->c_str(), 0666);
    }

    if (fd_ < 0)
      throwError(string("creat: ") + *file_ + " " + strerror(errno));
  }
}

void
CCommandFileDest::
initChild()
{
  if (command_->getDoFork()) {
    int error = close(dest_fd_);

    if (error < 0)
      throwError(string("close: ") + strerror(errno));

    error = dup2(fd_, dest_fd_);

    if (error < 0)
      throwError(string("dup2: ") + strerror(errno));

    error = close(fd_);

    if (error < 0)
      throwError(string("close: ") + strerror(errno));

    fd_ = -1;
  }
  else {
    save_fd_ = dup(dest_fd_);

    if (save_fd_ < 0)
      throwError(string("dup: ") + strerror(errno));

    int error = dup2(fd_, dest_fd_);

    if (error < 0)
      throwError(string("dup2: ") + strerror(errno));

    error = close(fd_);

    if (error < 0)
      throwError(string("close: ") + strerror(errno));

    fd_ = -1;
  }
}

void
CCommandFileDest::
term()
{
  if (fd_ != -1) {
    int error = close(fd_);

    if (error < 0)
      throwError(string("close: ") + strerror(errno));

    fd_ = -1;
  }

  if (save_fd_ != -1) {
    dup2(save_fd_, dest_fd_);

    close(save_fd_);

    save_fd_ = -1;
  }
}
