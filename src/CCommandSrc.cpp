#include "CCommandI.h"
#include <cstdio>

CCommandSrc::
CCommandSrc(CCommand *command) :
 command_(command)
{
  fd_ = -1;

  save_stdin_ = -1;
}

CCommandSrc::
CCommandSrc(CCommand *command, FILE *fp) :
 command_(command)
{
  fd_ = fileno(fp);
}

CCommandSrc::
~CCommandSrc()
{
}

void
CCommandSrc::
throwError(const std::string &msg)
{
  command_->throwError(msg);
}
