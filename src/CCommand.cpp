#include "CCommandI.h"
#include <COSProcess.h>
#include <COSSignal.h>
#include <COSTerm.h>
#include <cerrno>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <fcntl.h>

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
execCommand(const string &cmd)
{
  vector<string> words;

  CStrUtil::addWords(cmd, words);

  if (words.size() == 0)
    return false;

  string cmd1 = words[0];

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

list<CCommand *>
CCommandMgr::
getCommands()
{
  list<CCommand *> command_list;

  CCommandMgr::CommandMap::iterator p1, p2;

  for (p1 = CCommandMgrInst->commandsBegin(), p2 = CCommandMgrInst->commandsEnd(); p1 != p2; ++p1) {
    CCommand *command = (*p1).second;

    command_list.push_back(command);
  }

  return command_list;
}

list<CCommand *>
CCommandMgr::
getCommands(CCommand::State state)
{
  list<CCommand *> command_list;

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
throwError(const string &msg)
{
  last_error_ = msg;

  if (throwOnError_)
    CTHROW(msg);
}

//---------

CCommand::
CCommand(const string &cmdStr, bool do_fork) :
 name_         (cmdStr),
 path_         (""),
 do_fork_      (do_fork),
 callback_proc_(NULL),
 callback_data_(NULL),
 args_         (),
 pid_          (0),
 pgid_         (0),
 group_leader_ (false),
 group_id_     (0),
 child_        (false),
 state_        (IDLE_STATE),
 return_code_  (-1),
 signal_num_   (-1),
 src_list_     (),
 dest_list_    ()
{
  vector<string> args;

  init(args);
}

CCommand::
CCommand(const string &name, const string &path, const vector<string> &args, bool do_fork) :
 name_         (name),
 path_         (path),
 do_fork_      (do_fork),
 callback_proc_(NULL),
 callback_data_(NULL),
 args_         (),
 pid_          (0),
 pgid_         (0),
 group_leader_ (false),
 group_id_     (0),
 child_        (false),
 state_        (IDLE_STATE),
 return_code_  (-1),
 signal_num_   (-1),
 src_list_     (),
 dest_list_    ()
{
  init(args);
}

CCommand::
CCommand(const string &name, CallbackProc proc, CallbackData data,
         const vector<string> &args, bool do_fork) :
 name_         (name),
 path_         (""),
 do_fork_      (do_fork),
 callback_proc_(proc),
 callback_data_(data),
 args_         (),
 pid_          (0),
 pgid_         (0),
 group_leader_ (false),
 group_id_     (0),
 child_        (false),
 state_        (IDLE_STATE),
 return_code_  (-1),
 signal_num_   (-1),
 src_list_     (),
 dest_list_    ()
{
  init(args);
}

CCommand::
~CCommand()
{
  stop();

  CCommandMgrInst->deleteCommand(this);

  deleteSrcs();
  deleteDests();
}

void
CCommand::
init(const vector<string> &args)
{
  StringVectorT::const_iterator p1, p2;

  for (p1 = args.begin(), p2 = args.end(); p1 != p2; ++p1)
    args_.push_back(*p1);

  CCommandMgrInst->addCommand(this);
}

string
CCommand::
getCommandString() const
{
  string str = name_;

  uint num_args = args_.size();

  for (uint i = 0; i < num_args; ++i)
    str += " " + args_[i];

  return str;
}

void
CCommand::
addFileSrc(const string &filename)
{
  CCommandFileSrc *src = new CCommandFileSrc(this, filename);

  src_list_.push_back(src);
}

void
CCommand::
addFileSrc(FILE *fp)
{
  CCommandFileSrc *src = new CCommandFileSrc(this, fp);

  src_list_.push_back(src);
}

void
CCommand::
addPipeSrc()
{
  CCommandPipeDest *dest = CCommandMgrInst->getPipeDest();

  if (dest == NULL) {
    throwError("No Pipe Destination for Source");
    return;
  }

  CCommandPipeSrc *src = new CCommandPipeSrc(this);

  src_list_.push_back(src);

  src ->setDest(dest);
  dest->setSrc (src);

  CCommandMgrInst->setPipeDest(NULL);
}

void
CCommand::
addStringSrc(const string &str)
{
  CCommandStringSrc *src = new CCommandStringSrc(this, str);

  src_list_.push_back(src);
}

void
CCommand::
addFileDest(const string &filename, int fd)
{
  CCommandFileDest *dest = new CCommandFileDest(this, filename, fd);

  dest_list_.push_back(dest);
}

void
CCommand::
addFileDest(FILE *fp, int fd)
{
  CCommandFileDest *dest = new CCommandFileDest(this, fp, fd);

  dest_list_.push_back(dest);
}

void
CCommand::
addPipeDest(int fd)
{
  CCommandPipeDest *dest = CCommandMgrInst->getPipeDest();

  if (dest == NULL) {
    dest = new CCommandPipeDest(this);

    dest_list_.push_back(dest);

    CCommandMgrInst->setPipeDest(dest);
  }

  dest->addFd(fd);
}

void
CCommand::
addStringDest(string &str, int fd)
{
  CCommandStringDest *dest = new CCommandStringDest(this, str, fd);

  dest_list_.push_back(dest);
}

void
CCommand::
setFileDestOverwrite(bool overwrite, int fd)
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end(); p1 != p2; ++p1) {
    CCommandFileDest *dest = dynamic_cast<CCommandFileDest *>(*p1);

    if (dest != NULL && dest->getFd() == fd)
      dest->setOverwrite(overwrite);
  }
}

void
CCommand::
setFileDestAppend(bool append, int fd)
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end  (); p1 != p2; ++p1) {
    CCommandFileDest *dest = dynamic_cast<CCommandFileDest *>(*p1);

    if (dest != NULL && dest->getFd() == fd)
      dest->setAppend(append);
  }
}

void
CCommand::
start()
{
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Start command %s\n", name_.c_str());

  if (do_fork_) {
    initParentDests();
    initParentSrcs ();

    pid_ = fork();

    if      (pid_ < 0) {
      throwError(string("fork: ") + strerror(errno));
      return;
    }
    // child
    else if (pid_ == 0) {
      pid_ = COSProcess::getProcessId();

      updateProcessGroup();

      resetSignals();

      CCommandPipe::deleteOthers(this);

      child_ = true;

      initChildDests();
      initChildSrcs ();

      if (callback_proc_ == NULL)
        run();
      else {
        setReturnCode(0);

        callback_proc_(args_, callback_data_);

        setState(EXITED_STATE);
      }

      termSrcs ();
      termDests();

      died();

      _exit(255);
    }
    // parent
    else {
      updateProcessGroup();

      addSignals();

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %d\n", pid_);

      // setForegroundProcessGroup();

      setState(RUNNING_STATE);

      processSrcs ();
      processDests();
    }
  }
  else {
    initParentDests();
    initParentSrcs ();

    initChildDests();
    initChildSrcs ();

    setReturnCode(0);

    callback_proc_(args_, callback_data_);

    setState(EXITED_STATE);

    processDests();
    processSrcs ();

    termSrcs ();
    termDests();

    died();
  }
}

void
CCommand::
pause()
{
  if (isState(RUNNING_STATE)) {
    int error_code = COSSignal::sendSignal(pid_, SIGSTOP);

    if (error_code < 0) {
      throwError(string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
resume()
{
  if (isState(STOPPED_STATE)) {
    int error_code = COSSignal::sendSignal(pid_, SIGCONT);

    if (error_code < 0) {
      throwError(string("kill: ") + strerror(errno) + ".");
      return;
    }

    setState(RUNNING_STATE);
  }
}

void
CCommand::
stop()
{
  if (isState(RUNNING_STATE) || isState(STOPPED_STATE)) {
    int error_code = COSSignal::sendSignal(pid_, SIGTERM);

    if (error_code < 0) {
      throwError(string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
tstop()
{
  if (isState(RUNNING_STATE)) {
    int error_code = COSSignal::sendSignal(pid_, SIGTSTP);

    if (error_code < 0) {
      throwError(string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
wait()
{
  if (! do_fork_) {
    assert(isState(EXITED_STATE));
  }
  else {
    int fd = open("/dev/tty", O_RDWR);

    pid_t pgid = (fd != -1 ? tcgetpgrp(fd) : 0);

    if (fd != -1 && pgid_ != pgid) {
      COSSignal::ignoreSignal(SIGTTOU);

      tcsetpgrp(fd, pgid_);

      COSSignal::defaultSignal(SIGTTOU);
    }

    while (! isState(EXITED_STATE) && ! isState(STOPPED_STATE))
      wait_pid(pid_, false);

    if (fd != -1 && pgid_ != pgid) {
      COSSignal::ignoreSignal(SIGTTOU);

      tcsetpgrp(fd, pgid);

      COSSignal::defaultSignal(SIGTTOU);
    }

    if (fd != -1)
      close(fd);
  }
}

void
CCommand::
waitpid()
{
  while (! isState(EXITED_STATE) && ! isState(STOPPED_STATE))
    wait_pid(pid_, false);
}

void
CCommand::
waitpgid()
{
  assert(pgid_);

  while (! isState(EXITED_STATE) && ! isState(STOPPED_STATE))
    wait_pid(-pgid_, false);
}

void
CCommand::
addSignals()
{
  COSSignal::addSignalHandler(SIGCHLD, (COSSignal::SignalHandler) signalChild  );
  COSSignal::addSignalHandler(SIGPIPE, (COSSignal::SignalHandler) signalGeneric);
}

void
CCommand::
resetSignals()
{
  COSSignal::defaultSignal(SIGHUP  );
  COSSignal::defaultSignal(SIGINT  );
  COSSignal::defaultSignal(SIGQUIT );
  COSSignal::defaultSignal(SIGILL  );
  COSSignal::defaultSignal(SIGTRAP );
  COSSignal::defaultSignal(SIGIOT  );
  COSSignal::defaultSignal(SIGFPE  );
  COSSignal::defaultSignal(SIGKILL );
  COSSignal::defaultSignal(SIGUSR1 );
  COSSignal::defaultSignal(SIGUSR2 );
  COSSignal::defaultSignal(SIGPIPE );
  COSSignal::defaultSignal(SIGALRM );
  COSSignal::defaultSignal(SIGTERM );
  COSSignal::defaultSignal(SIGCHLD );
  COSSignal::defaultSignal(SIGCONT );
  COSSignal::defaultSignal(SIGSTOP );
  COSSignal::defaultSignal(SIGTTIN );
  COSSignal::defaultSignal(SIGTTOU );
  COSSignal::defaultSignal(SIGWINCH);

  COSSignal::addSignalHandler(SIGTSTP , (COSSignal::SignalHandler) signalStop);
}

void
CCommand::
signalChild(int)
{
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("SignalChild\n");

  wait_pid(-1, true);

  CCommandMgr::CommandMap::iterator p1, p2;

  for (p1 = CCommandMgrInst->commandsBegin(), p2 = CCommandMgrInst->commandsEnd(); p1 != p2; ++p1) {
    CCommand *command = (*p1).second;

    if (command->isState(EXITED_STATE))
      continue;

    wait_pid(command->pid_, true);
  }
}

void
CCommand::
wait_pid(pid_t pid, bool nohang)
{
  CCommand *command = NULL;

  if (pid > 0)
    command = CCommandMgrInst->lookup(pid);

  if (CCommandMgrInst->getDebug()) {
    if (pid > 0) {
      if (command)
        CCommandUtil::outputMsg("Waiting for process %s\n", command->name_.c_str());
      else
        CCommandUtil::outputMsg("Waiting for process %d\n", pid);
    }
    else {
      if (pid == -1)
        CCommandUtil::outputMsg("Waiting for all children\n");
      else
        CCommandUtil::outputMsg("Waiting for process group %d\n", -pid);
    }
  }

  int status;

  int flags = WUNTRACED;

#ifdef WCONTINUED
  flags |= WCONTINUED;
#endif

  if (nohang)
    flags |= WNOHANG;

  pid_t wait_pid = ::waitpid(pid, &status, flags);

  if (nohang && wait_pid == 0)
    return;

  if (pid == -1 && wait_pid > 0)
    command = CCommandMgrInst->lookup(wait_pid);

  if (wait_pid > 0) {
    if (command == NULL)
      return;

    if      (WIFEXITED(status)) {
      int return_code = WEXITSTATUS(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Exited %d\n", command->name_.c_str(),
                                return_code);

      command->setReturnCode(return_code);
      command->setState     (EXITED_STATE);

      command->termSrcs();
      command->termDests();

      command->died();
    }
    else if (WIFSTOPPED(status)) {
      int signal_num = WSTOPSIG(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Stopped '%s'(%d)\n", command->name_.c_str(),
                                COSSignal::strsignal(signal_num).c_str(), signal_num);

      command->setSignalNum(signal_num);
      command->setState    (STOPPED_STATE);
    }
    else if (WIFSIGNALED(status)) {
      int signal_num = WTERMSIG(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Signalled '%s'(%d)\n", command->name_.c_str(),
                                COSSignal::strsignal(signal_num).c_str(), signal_num);

      command->setSignalNum(signal_num);
      command->setState    (SIGNALLED_STATE);
    }
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(status)) {
      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Continued\n", command->name_.c_str());

      command->setState(RUNNING_STATE);
    }
#endif
  }
  else {
    if (errno == ECHILD) {
      if (command != NULL) {
        if (CCommandMgrInst->getDebug())
          CCommandUtil::outputMsg("Process %s Does Not Exist\n", command->name_.c_str());

        int return_code = -1;

        command->setReturnCode(return_code);
        command->setState     (EXITED_STATE);

        command->termSrcs();
        command->termDests();

        command->died();
      }
      else {
        if (CCommandMgrInst->getDebug())
          CCommandUtil::outputMsg("No matching command for ECHILD from waitpid\n");
      }
    }
    else if (errno == EINTR) {
      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Interrrupted System Call\n");
    }
    else {
      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Unknown error from waitpid\n");
    }
  }
}

void
CCommand::
signalGeneric(int sig)
{
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Signal '%s'(%d) received\n", COSSignal::strsignal(sig).c_str(), sig);
}

void
CCommand::
signalStop(int)
{
  COSSignal::sendSignalToProcessGroup(SIGSEGV);
}

void
CCommand::
run()
{
  int num_args = args_.size();

  char **args = new char * [num_args + 2];

  int i = 0;

  args[i++] = (char *) name_.c_str();

  StringVectorT::iterator p1, p2;

  for (p1 = args_.begin(), p2 = args_.end(); p1 != p2; ++p1)
    args[i++] = (char *) (*p1).c_str();

  args[i] = NULL;

  // setpgrp();

  // TODO: use execve to avoid PATH lookup

  int error = execvp(args[0], args);

  if (error != 0) {
    throwError(string("execvp: ") + args[0] + " " + strerror(errno));
    return;
  }

  _exit(255);
}

void
CCommand::
initParentSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = src_list_.begin(), p2 = src_list_.end(); p1 != p2; ++p1)
    (*p1)->initParent();
}

void
CCommand::
initParentDests()
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end(); p1 != p2; ++p1)
    (*p1)->initParent();
}

void
CCommand::
initChildSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = src_list_.begin(), p2 = src_list_.end(); p1 != p2; ++p1)
    (*p1)->initChild();
}

void
CCommand::
initChildDests()
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end(); p1 != p2; ++p1)
    (*p1)->initChild();
}

void
CCommand::
processSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = src_list_.begin(), p2 = src_list_.end(); p1 != p2; ++p1)
    (*p1)->process();
}

void
CCommand::
processDests()
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end(); p1 != p2; ++p1)
    (*p1)->process();
}

void
CCommand::
termSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = src_list_.begin(), p2 = src_list_.end(); p1 != p2; ++p1)
    (*p1)->term();
}

void
CCommand::
termDests()
{
  DestList::iterator p1, p2;

  for (p1 = dest_list_.begin(), p2 = dest_list_.end(); p1 != p2; ++p1)
    (*p1)->term();
}

void
CCommand::
deleteSrcs()
{
  std::for_each(src_list_.begin(), src_list_.end(), CDeletePointer());

  src_list_.clear();
}

void
CCommand::
deleteDests()
{
  std::for_each(dest_list_.begin(), dest_list_.end(), CDeletePointer());

  dest_list_.clear();
}

void
CCommand::
setForegroundProcessGroup()
{
  static pid_t pgrp;
  static bool  initialized = false;

  if (! initialized) {
    string tty = COSTerm::getTerminalName();

    pgrp = COSTerm::getTerminalProcessGroupId(tty);

    if (pgrp == -1) {
      throwError("Failed to get process group\n");
      return;
    }

    initialized = true;
  }

  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Set process group to %d\n", pgrp);

  COSProcess::setProcessGroupId(pid_, pgrp);
}

void
CCommand::
setProcessGroupLeader()
{
  group_leader_ = true;
  group_id_     = 0;

  pgid_ = pid_;

  if (! pid_)
    COSProcess::setProcessGroupId(pid_);
}

void
CCommand::
setProcessGroup(CCommand *group_command)
{
  group_leader_ = false;
  group_id_     = group_command->getId();

  if (group_command->pid_ != 0)
    pgid_ = group_command->pid_;

  if (! pid_ && ! pgid_)
    COSProcess::setProcessGroupId(pid_, pgid_);
}

void
CCommand::
updateProcessGroup()
{
  assert(pid_);

  if      (group_leader_) {
    pgid_ = pid_;

    COSProcess::setProcessGroupId(pid_);
  }
  else if (group_id_) {
    CCommand *group_command = CCommandMgrInst->getCommand(group_id_);

    if (! group_command) return;

    pgid_ = group_command->pid_;

    COSProcess::setProcessGroupId(pid_, pgid_);
  }
}

void
CCommand::
setState(State state)
{
  if (isState(state))
    return;

  state_ = state;
}

void
CCommand::
throwError(const string &msg)
{
  CCommandMgrInst->throwError(msg);
}
