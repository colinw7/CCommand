#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <cassert>

using StringVectorT = std::vector<std::string>;

class CCommand;
class CCommandPipe;
class CCommandSrc;
class CCommandDest;
class CCommandPipeDest;

class CCommand {
 public:
  using SrcList  = std::list<CCommandSrc *>;
  using DestList = std::list<CCommandDest *>;
  using Args     = StringVectorT;

  using CallbackData = void *;
  using CallbackProc = void (*)(const Args &args, CallbackData data);

  enum class State {
    NONE,
    IDLE,
    RUNNING,
    EXITED,
    SIGNALLED,
    STOPPED
  };

 public:
  CCommand(const std::string &cmdStr, bool doFork=true);

  CCommand(const std::string &name, const std::string &path,
           const Args &args=Args(), bool doFork=true);

  CCommand(const std::string &name, CallbackProc proc, CallbackData data,
           const Args &args=Args(), bool doFork=false);

  virtual ~CCommand();

  const std::string &getName() const { return name_; }
  const std::string &getPath() const { return path_; }

  uint getId() const { return id_; }
  void setId(uint id) { id_ = id; }

  bool getDoFork() const { return doFork_; }
  void setDoFork(bool doFork) { doFork_ = doFork; }

  CallbackProc getCallbackProc() const { return callbackProc_; }
  CallbackData getCallbackData() const { return callbackData_; }

  const Args        &getArgs  ()      const { return args_; }
  int               getNumArgs()      const { return int(args_.size()); }
  const std::string &getArg   (int i) const { assert(i >= 0); return args_[size_t(i)]; }

  void addArg(const std::string &arg) { args_.push_back(arg); }

  pid_t getPid() const { return pid_ ; }

  bool isChild() const { return child_; }

  State getState() const { return state_; }
  bool  isState(State state) const { return (state_ == state); }

  int getReturnCode() const { return returnCode_; }

  int getSignalNum() const { return signalNum_ ; }

  std::string getCommandString() const;

  //---

  // add source (file, pipe output, string)
  void addFileSrc(const std::string &filename);
  void addFileSrc(FILE *fp);

  void addPipeSrc();

  void addStringSrc(const std::string &str);

  // add dest (file, pipe input, string)
  void addFileDest(const std::string &filename, int fd=1);
  void addFileDest(FILE *fp, int fd=1);

  void addPipeDest(int fd=1);

  void addStringDest(std::string &str, int fd=1);

  //--

  // set dest overwrite/depend
  void setFileDestOverwrite(bool overwrite, int fd=1);
  void setFileDestAppend(bool append, int fd=1);

  //---

  void start ();
  void stop  ();
  void tstop ();
  void pause ();
  void resume();
  void wait  ();

  void waitpid();
  void waitpgid();

  void setProcessGroupLeader();
  void setProcessGroup(CCommand *command);

  void updateProcessGroup();

  void throwError(const std::string &msg);

 protected:
  virtual void run();
  virtual void died() { }

  virtual void setState(State state);

 private:
  void init(const Args &args);

  void initParentSrcs();
  void initParentDests();

  void initChildSrcs();
  void initChildDests();

  void processSrcs();
  void processDests();

  void termSrcs();
  void termDests();

  void deleteSrcs();
  void deleteDests();

  void addSignals();
  void resetSignals();

  void setReturnCode(int returnCode) { returnCode_ = returnCode; }

  void setSignalNum(int signalNum) { signalNum_ = signalNum; }

  void setForegroundProcessGroup();

  static void signalChild  (int sig);
  static void signalGeneric(int sig);
  static void signalStop   (int sig);

  static void wait_pid(pid_t pid, bool nohang);

 private:
  std::string  name_;
  std::string  path_;
  uint         id_           { 0 };
  bool         doFork_       { false };
  CallbackProc callbackProc_ { nullptr };
  CallbackData callbackData_ { nullptr };
  Args         args_;
  pid_t        pid_          { 0 };
  pid_t        pgid_         { 0 };
  bool         groupLeader_  { false };
  uint         groupId_      { 0 };
  bool         child_        { false };

  State        state_        { State::NONE };
  int          returnCode_   { -1 };
  int          signalNum_    { -1 };

  SrcList      srcList_;
  DestList     destList_;
};

#endif
