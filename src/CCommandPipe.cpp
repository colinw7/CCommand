#include "CCommandI.h"
#include <cerrno>
#include <cstring>

list<CCommandPipe *> CCommandPipe::pipes_;

CCommandPipe::
CCommandPipe(CCommand *command) :
 command_(command), src_(NULL), dest_(NULL)
{
  int error = ::pipe(fd_);

  if (error < 0)
    throwError(string("pipe: ") + strerror(errno));

  pipes_.push_back(this);
}

CCommandPipe::
~CCommandPipe()
{
  int error = closeInput();

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

  error = closeOutput();

  if (error < 0)
    throwError(string("close: ") + strerror(errno));

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
  list<CCommandPipe *>::iterator ppipe = pipes_.begin();

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
throwError(const string &msg)
{
  command_->throwError(msg);
}
