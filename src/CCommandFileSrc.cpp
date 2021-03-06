#include <CCommandFileSrc.h>
#include <CCommand.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

CCommandFileSrc::
CCommandFileSrc(CCommand *command, const std::string &file) :
 CCommandSrc(command)
{
  file_ = new std::string(file);
}

CCommandFileSrc::
CCommandFileSrc(CCommand *command, FILE *fp) :
 CCommandSrc(command, fp)
{
}

CCommandFileSrc::
~CCommandFileSrc()
{
  delete file_;

  term();
}

void
CCommandFileSrc::
initParent()
{
  if (file_) {
    fd_ = open(file_->c_str(), O_RDONLY);

    if (fd_ < 0)
      throwError(std::string("open: ") + *file_ + " " + strerror(errno));
  }
}

void
CCommandFileSrc::
initChild()
{
  if (command_->getDoFork()) {
    int error = close(0);

    if (error < 0)
      throwError(std::string("close: ") + strerror(errno));

    error = dup2(fd_, 0);

    if (error < 0)
      throwError(std::string("dup2: ") + strerror(errno));

    error = close(fd_);

    if (error < 0)
      throwError(std::string("close: ") + strerror(errno));
  }
  else {
    save_stdin_ = dup(0);

    if (save_stdin_ < 0)
      throwError(std::string("dup: ") + strerror(errno));

    int error = dup2(fd_, 0);

    if (error < 0)
      throwError(std::string("dup2: ") + strerror(errno));
  }
}

void
CCommandFileSrc::
term()
{
  if (fd_ != -1) {
    int error = close(fd_);

    if (error < 0)
      throwError(std::string("close: ") + strerror(errno));

    fd_ = -1;
  }

  if (save_stdin_ != -1) {
    dup2(save_stdin_, 0);

    close(save_stdin_);

    save_stdin_ = -1;
  }
}
