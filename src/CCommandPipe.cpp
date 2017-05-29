#include <CCommandPipe.h>
#include <CCommand.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>

std::list<CCommandPipe *> CCommandPipe::pipes_;

CCommandPipe::
CCommandPipe(CCommand *command) :
 command_(command)
{
  int error = ::pipe(fd_);

  if (error < 0)
    throwError(std::string("pipe: ") + strerror(errno));

  pipes_.push_back(this);
}

CCommandPipe::
~CCommandPipe()
{
  int error = closeInput();

  if (error < 0)
    throwError(std::string("close: ") + strerror(errno));

  error = closeOutput();

  if (error < 0)
    throwError(std::string("close: ") + strerror(errno));

  pipes_.remove(this);
}

int
CCommandPipe::
closeInput()
{
  int error = 0;

  if (fd_[0] != -1)
    error = close(fd_[0]);

  fd_[0] = -1;

  return error;
}

int
CCommandPipe::
closeOutput()
{
  int error = 0;

  if (fd_[1] != -1)
    error = close(fd_[1]);

  fd_[1] = -1;

  return error;
}

void
CCommandPipe::
deleteOthers(CCommand *command)
{
  std::list<CCommandPipe *>::iterator ppipe = pipes_.begin();

  while (ppipe != pipes_.end()) {
    if ((*ppipe)->src_ != command && (*ppipe)->dest_ != command) {
      delete *ppipe;

      ppipe = pipes_.begin();
    }
    else
      ++ppipe;
  }
}

void
CCommandPipe::
throwError(const std::string &msg)
{
  command_->throwError(msg);
}
