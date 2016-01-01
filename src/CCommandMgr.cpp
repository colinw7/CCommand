#include <CCommandMgr.h>
#include <CStrUtil.h>
#include <CThrow.h>

CCommandMgr::
CCommandMgr() :
 command_map_ (),
 pipe_dest_   (NULL),
 last_error_  (""),
 last_id_     (0),
 throwOnError_(false),
 debug_       (false)
{
}

void
CCommandMgr::
addCommand(CCommand *command)
{
  command->setId(++last_id_);

  command_map_[last_id_] = command;
}

void
CCommandMgr::
deleteCommand(CCommand *command)
{
  command_map_.erase(command->getId());
}

CCommand *
CCommandMgr::
getCommand(uint id) const
{
  CommandMap::const_iterator p = command_map_.find(id);

  if (p == command_map_.end()) return NULL;

  return (*p).second;
}

bool
CCommandMgr::
execCommand(const std::string &cmd)
{
  std::vector<std::string> words;

  CStrUtil::addWords(cmd, words);

  if (words.size() == 0)
    return false;

  std::string cmd1 = words[0];

  words.erase(words.begin());

  CCommand command(cmd1, cmd1, words);

  command.start();

  return true;
}

CCommand *
CCommandMgr::
lookup(pid_t pid)
{
  CommandMap::iterator p1, p2;

  for (p1 = commandsBegin(), p2 = commandsEnd(); p1 != p2; ++p1) {
    CCommand *command = (*p1).second;

    if (command->getPid() == pid)
      return command;
  }

  return NULL;
}

std::list<CCommand *>
CCommandMgr::
getCommands()
{
  std::list<CCommand *> command_list;

  CCommandMgr::CommandMap::iterator p1, p2;

  for (p1 = CCommandMgrInst->commandsBegin(), p2 = CCommandMgrInst->commandsEnd(); p1 != p2; ++p1) {
    CCommand *command = (*p1).second;

    command_list.push_back(command);
  }

  return command_list;
}

std::list<CCommand *>
CCommandMgr::
getCommands(CCommand::State state)
{
  std::list<CCommand *> command_list;

  CCommandMgr::CommandMap::iterator p1, p2;

  for (p1 = CCommandMgrInst->commandsBegin(), p2 = CCommandMgrInst->commandsEnd(); p1 != p2; ++p1) {
    CCommand *command = (*p1).second;

    if (command->isState(state))
      command_list.push_back(command);
  }

  return command_list;
}

void
CCommandMgr::
throwError(const std::string &msg)
{
  last_error_ = msg;

  if (throwOnError_)
    CTHROW(msg);
}
