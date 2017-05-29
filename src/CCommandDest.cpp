#include <CCommandDest.h>
#include <CCommand.h>

CCommandDest::
CCommandDest(CCommand *command) :
 command_(command)
{
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
