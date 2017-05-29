#include <CCommandSrc.h>
#include <CCommand.h>

CCommandSrc::
CCommandSrc(CCommand *command) :
 command_(command)
{
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
