#include "CCommandI.h"
#include <cerrno>
#include <cstring>

CCommandStringSrc::
CCommandStringSrc(CCommand *command, const string &str) :
 CCommandSrc(command), str_(str)
{
  pipe_ = NULL;
}

CCommandStringSrc::
~CCommandStringSrc()
{
  term();

  delete pipe_;
}

void
CCommandStringSrc::
initParent()
{
  pipe_ = new CCommandPipe(command_);

  save_stdin_ = dup(0);

  if (save_stdin_ < 0)
    throwError(string("dup: ") + strerror(errno));

  int error = close(0);

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  error = dup2(pipe_->getInput(), 0);

  if (error < 0)
    throwError(string("dup2: ") + strerror(errno));
}

void
CCommandStringSrc::
initChild()
{
}

void
CCommandStringSrc::
term()
{
}

void
CCommandStringSrc::
process()
{
  if (save_stdin_ != -1) {
    dup2(save_stdin_, 0);

    close(save_stdin_);

    save_stdin_ = -1;
  }

  int error = pipe_->closeInput();

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  int fd = pipe_->getOutput();

  if (fd != -1) {
    int num_written = write(pipe_->getOutput(), str_.c_str(), str_.size());

    if (num_written != (int) str_.size())
      throwError(string("write: ") + strerror(errno));

    int error = pipe_->closeOutput();

    if (error < 0)
      throwError(string("close: ") + strerror(errno));
  }
}
