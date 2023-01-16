#include <CCommand.h>
#include <CCommandMgr.h>
#include <CCommandFileSrc.h>
#include <CCommandFileDest.h>
#include <CCommandPipeSrc.h>
#include <CCommandPipeDest.h>
#include <CCommandStringSrc.h>
#include <CCommandStringDest.h>
#include <CCommandPipe.h>
#include <CCommandUtil.h>
#include <COSProcess.h>
#include <COSSignal.h>
#include <COSTerm.h>
#include <CFuncs.h>

#include <algorithm>
#include <cerrno>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

CCommand::
CCommand(const std::string &cmdStr, bool doFork) :
 name_   (cmdStr),
 doFork_(doFork),
 state_  (State::IDLE)
{
  std::vector<std::string> args;

  init(args);
}

CCommand::
CCommand(const std::string &name, const std::string &path,
         const std::vector<std::string> &args, bool doFork) :
 name_   (name),
 path_   (path),
 doFork_(doFork),
 state_  (State::IDLE)
{
  init(args);
}

CCommand::
CCommand(const std::string &name, CallbackProc proc, CallbackData data,
         const std::vector<std::string> &args, bool doFork) :
 name_        (name),
 doFork_      (doFork),
 callbackProc_(proc),
 callbackData_(data),
 state_       (State::IDLE)
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
init(const std::vector<std::string> &args)
{
  for (const auto &arg : args)
    args_.push_back(arg);

  CCommandMgrInst->addCommand(this);
}

std::string
CCommand::
getCommandString() const
{
  std::string str = name_;

  auto numArgs = args_.size();

  for (uint i = 0; i < numArgs; ++i)
    str += " " + args_[i];

  return str;
}

void
CCommand::
addFileSrc(const std::string &filename)
{
  auto *src = new CCommandFileSrc(this, filename);

  srcList_.push_back(src);
}

void
CCommand::
addFileSrc(FILE *fp)
{
  auto *src = new CCommandFileSrc(this, fp);

  srcList_.push_back(src);
}

void
CCommand::
addPipeSrc()
{
  auto *dest = CCommandMgrInst->getPipeDest();

  if (dest == nullptr) {
    throwError("No Pipe Destination for Source");
    return;
  }

  auto *src = new CCommandPipeSrc(this);

  srcList_.push_back(src);

  src ->setDest(dest);
  dest->setSrc (src);

  CCommandMgrInst->setPipeDest(nullptr);
}

void
CCommand::
addStringSrc(const std::string &str)
{
  auto *src = new CCommandStringSrc(this, str);

  srcList_.push_back(src);
}

void
CCommand::
addFileDest(const std::string &filename, int fd)
{
  auto *dest = new CCommandFileDest(this, filename, fd);

  destList_.push_back(dest);
}

void
CCommand::
addFileDest(FILE *fp, int fd)
{
  auto *dest = new CCommandFileDest(this, fp, fd);

  destList_.push_back(dest);
}

void
CCommand::
addPipeDest(int fd)
{
  auto *dest = CCommandMgrInst->getPipeDest();

  if (dest == nullptr) {
    dest = new CCommandPipeDest(this);

    destList_.push_back(dest);

    CCommandMgrInst->setPipeDest(dest);
  }

  dest->addFd(fd);
}

void
CCommand::
addStringDest(std::string &str, int fd)
{
  auto *dest = new CCommandStringDest(this, str, fd);

  destList_.push_back(dest);
}

void
CCommand::
setFileDestOverwrite(bool overwrite, int fd)
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end(); p1 != p2; ++p1) {
    auto *dest = dynamic_cast<CCommandFileDest *>(*p1);

    if (dest != nullptr && dest->getFd() == fd)
      dest->setOverwrite(overwrite);
  }
}

void
CCommand::
setFileDestAppend(bool append, int fd)
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end  (); p1 != p2; ++p1) {
    auto *dest = dynamic_cast<CCommandFileDest *>(*p1);

    if (dest != nullptr && dest->getFd() == fd)
      dest->setAppend(append);
  }
}

void
CCommand::
start()
{
  if (CCommandMgrInst->getDebug())
    CCommandUtil::outputMsg("Start command %s\n", name_.c_str());

  if (doFork_) {
    initParentDests();
    initParentSrcs ();

    pid_ = fork();

    if      (pid_ < 0) {
      throwError(std::string("fork: ") + strerror(errno));
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

      if (! callbackProc_)
        run();
      else {
        setReturnCode(0);

        callbackProc_(args_, callbackData_);

        setState(State::EXITED);
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

      setState(State::RUNNING);

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

    assert(callbackProc_);

    callbackProc_(args_, callbackData_);

    setState(State::EXITED);

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
  if (isState(State::RUNNING)) {
    int errorCode = COSSignal::sendSignal(pid_, SIGSTOP);

    if (errorCode < 0) {
      throwError(std::string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
resume()
{
  if (isState(State::STOPPED)) {
    int errorCode = COSSignal::sendSignal(pid_, SIGCONT);

    if (errorCode < 0) {
      throwError(std::string("kill: ") + strerror(errno) + ".");
      return;
    }

    setState(State::RUNNING);
  }
}

void
CCommand::
stop()
{
  if (isState(State::RUNNING) || isState(State::STOPPED)) {
    int errorCode = COSSignal::sendSignal(pid_, SIGTERM);

    if (errorCode < 0) {
      throwError(std::string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
tstop()
{
  if (isState(State::RUNNING)) {
    int errorCode = COSSignal::sendSignal(pid_, SIGTSTP);

    if (errorCode < 0) {
      throwError(std::string("kill: ") + strerror(errno) + ".");
      return;
    }
  }
}

void
CCommand::
wait()
{
  if (! doFork_) {
    assert(isState(State::EXITED));
  }
  else {
    int fd = open("/dev/tty", O_RDWR);

    pid_t pgid = (fd != -1 ? tcgetpgrp(fd) : 0);

    if (fd != -1 && pgid_ != pgid) {
      COSSignal::ignoreSignal(SIGTTOU);

      tcsetpgrp(fd, pgid_);

      COSSignal::defaultSignal(SIGTTOU);
    }

    while (! isState(State::EXITED) && ! isState(State::STOPPED))
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
  while (! isState(State::EXITED) && ! isState(State::STOPPED))
    wait_pid(pid_, false);
}

void
CCommand::
waitpgid()
{
  assert(pgid_);

  while (! isState(State::EXITED) && ! isState(State::STOPPED))
    wait_pid(-pgid_, false);
}

void
CCommand::
addSignals()
{
  COSSignal::addSignalHandler(SIGCHLD, COSSignal::SignalHandler(signalChild  ));
  COSSignal::addSignalHandler(SIGPIPE, COSSignal::SignalHandler(signalGeneric));
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

  COSSignal::addSignalHandler(SIGTSTP , COSSignal::SignalHandler(signalStop));
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

    if (command->isState(State::EXITED))
      continue;

    wait_pid(command->pid_, true);
  }
}

void
CCommand::
wait_pid(pid_t pid, bool nohang)
{
  CCommand *command = nullptr;

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
    if (command == nullptr)
      return;

    if      (WIFEXITED(status)) {
      int returnCode = WEXITSTATUS(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Exited %d\n", command->name_.c_str(), returnCode);

      command->setReturnCode(returnCode);
      command->setState     (State::EXITED);

      command->termSrcs();
      command->termDests();

      command->died();
    }
    else if (WIFSTOPPED(status)) {
      int signalNum = WSTOPSIG(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Stopped '%s'(%d)\n", command->name_.c_str(),
                                COSSignal::strsignal(signalNum).c_str(), signalNum);

      command->setSignalNum(signalNum);
      command->setState    (State::STOPPED);
    }
    else if (WIFSIGNALED(status)) {
      int signalNum = WTERMSIG(status);

      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Signalled '%s'(%d)\n", command->name_.c_str(),
                                COSSignal::strsignal(signalNum).c_str(), signalNum);

      command->setSignalNum(signalNum);
      command->setState    (State::SIGNALLED);
    }
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(status)) {
      if (CCommandMgrInst->getDebug())
        CCommandUtil::outputMsg("Process %s Continued\n", command->name_.c_str());

      command->setState(State::RUNNING);
    }
#endif
  }
  else {
    if (errno == ECHILD) {
      if (command != nullptr) {
        if (CCommandMgrInst->getDebug())
          CCommandUtil::outputMsg("Process %s Does Not Exist\n", command->name_.c_str());

        int returnCode = -1;

        command->setReturnCode(returnCode);
        command->setState     (State::EXITED);

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
  auto numArgs = args_.size();

  auto **args = new char * [numArgs + 2];

  int i = 0;

  args[i++] = const_cast<char *>(name_.c_str());

  StringVectorT::iterator p1, p2;

  for (p1 = args_.begin(), p2 = args_.end(); p1 != p2; ++p1)
    args[i++] = const_cast<char *>((*p1).c_str());

  args[i] = nullptr;

  // setpgrp();

  // TODO: use execve to avoid PATH lookup
  int error = execvp(args[0], args);

  if (error != 0) {
    throwError(std::string("execvp: ") + args[0] + " " + strerror(errno));
    return;
  }

  _exit(255);
}

void
CCommand::
initParentSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = srcList_.begin(), p2 = srcList_.end(); p1 != p2; ++p1)
    (*p1)->initParent();
}

void
CCommand::
initParentDests()
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end(); p1 != p2; ++p1)
    (*p1)->initParent();
}

void
CCommand::
initChildSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = srcList_.begin(), p2 = srcList_.end(); p1 != p2; ++p1)
    (*p1)->initChild();
}

void
CCommand::
initChildDests()
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end(); p1 != p2; ++p1)
    (*p1)->initChild();
}

void
CCommand::
processSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = srcList_.begin(), p2 = srcList_.end(); p1 != p2; ++p1)
    (*p1)->process();
}

void
CCommand::
processDests()
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end(); p1 != p2; ++p1)
    (*p1)->process();
}

void
CCommand::
termSrcs()
{
  SrcList::iterator p1, p2;

  for (p1 = srcList_.begin(), p2 = srcList_.end(); p1 != p2; ++p1)
    (*p1)->term();
}

void
CCommand::
termDests()
{
  DestList::iterator p1, p2;

  for (p1 = destList_.begin(), p2 = destList_.end(); p1 != p2; ++p1)
    (*p1)->term();
}

void
CCommand::
deleteSrcs()
{
  std::for_each(srcList_.begin(), srcList_.end(), CDeletePointer());

  srcList_.clear();
}

void
CCommand::
deleteDests()
{
  std::for_each(destList_.begin(), destList_.end(), CDeletePointer());

  destList_.clear();
}

void
CCommand::
setForegroundProcessGroup()
{
  static pid_t pgrp;
  static bool  initialized = false;

  if (! initialized) {
    std::string tty = COSTerm::getTerminalName();

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
  groupLeader_ = true;
  groupId_     = 0;

  pgid_ = pid_;

  if (! pid_)
    COSProcess::setProcessGroupId(pid_);
}

void
CCommand::
setProcessGroup(CCommand *groupCommand)
{
  groupLeader_ = false;
  groupId_     = groupCommand->getId();

  if (groupCommand->pid_ != 0)
    pgid_ = groupCommand->pid_;

  if (! pid_ && ! pgid_)
    COSProcess::setProcessGroupId(pid_, pgid_);
}

void
CCommand::
updateProcessGroup()
{
  assert(pid_);

  if      (groupLeader_) {
    pgid_ = pid_;

    COSProcess::setProcessGroupId(pid_);
  }
  else if (groupId_) {
    auto *groupCommand = CCommandMgrInst->getCommand(groupId_);

    if (! groupCommand) return;

    pgid_ = groupCommand->pid_;

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
throwError(const std::string &msg)
{
  CCommandMgrInst->throwError(msg);
}
