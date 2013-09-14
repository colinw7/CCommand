#include "CCommandI.h"
#include <cstdio>

CCommandDest::
CCommandDest(CCommand *command) :
 command_(command)
{
  fd_      = -1;
  save_fd_ = -1;
}

CCommandDest::
CCommandDest(CCommand *command, FILE *fp) :
 command_(command)
{
  fd_ = fileno(fp);
}

CCommandDest::
~CCommandDest()
{
}

void
CCommandDest::
throwError(const std::string &msg)
{
  command_->throwError(msg);
}
