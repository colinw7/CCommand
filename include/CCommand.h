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

  typedef void  *CallbackData;
  typedef void (*CallbackProc)(const Args &args, CallbackData data);

  enum class State {
    NONE,
    IDLE,
    RUNNING,
    EXITED,
    SIGNALLED,
    STOPPED
  };

 public:
  CCommand(const std::string &cmdStr, bool do_fork=true);

  CCommand(const std::string &name, const std::string &path,
           const Args &args=Args(), bool do_fork=true);

  CCommand(const std::string &name, CallbackProc proc, CallbackData data,
           const Args &args=Args(), bool do_fork=false);

  virtual ~CCommand();

  const std::string &getName() const { return name_; }
  const std::string &getPath() const { return path_; }

  uint getId() const { return id_; }
  void setId(uint id) { id_ = id; }

  bool getDoFork() const { return do_fork_; }
  void setDoFork(bool do_fork) { do_fork_ = do_fork; }

  CallbackProc getCallbackProc() const { return callback_proc_; }
  CallbackData getCallbackData() const { return callback_data_; }

  const Args        &getArgs  ()      const { return args_; }
  int               getNumArgs()      const { return int(args_.size()); }
  const std::string &getArg   (int i) const { assert(i >= 0); return args_[size_t(i)]; }

  void addArg(const std::string &arg) { args_.push_back(arg); }

  pid_t getPid () const { return pid_ ; }

  bool isChild() const { return child_; }

  State getState() const { return state_; }
  bool  isState(State state) const { return (state_ == state); }

  int getReturnCode() const { return return_code_; }

  int getSignalNum () const { return signal_num_ ; }

  std::string getCommandString() const;

  void addFileSrc(const std::string &filename);
  void addFileSrc(FILE *fp);

  void addPipeSrc();

  void addStringSrc(const std::string &str);

  void addFileDest(const std::string &filename, int fd=1);
  void addFileDest(FILE *fp, int fd=1);

  void addPipeDest(int fd=1);

  void addStringDest(std::string &str, int fd=1);

  void setFileDestOverwrite(bool overwrite, int fd=1);
  void setFileDestAppend(bool append, int fd=1);

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

  void setReturnCode(int return_code) { return_code_ = return_code; }

  void setSignalNum(int signal_num) { signal_num_ = signal_num; }

  void setForegroundProcessGroup();

  static void signalChild  (int sig);
  static void signalGeneric(int sig);
  static void signalStop   (int sig);

  static void wait_pid(pid_t pid, bool nohang);

 private:
  std::string  name_;
  std::string  path_;
  uint         id_            { 0 };
  bool         do_fork_       { false };
  CallbackProc callback_proc_ { nullptr };
  CallbackData callback_data_ { nullptr };
  Args         args_;
  pid_t        pid_           { 0 };
  pid_t        pgid_          { 0 };
  bool         group_leader_  { false };
  uint         group_id_      { 0 };
  bool         child_         { false };

  State        state_         { State::NONE };
  int          return_code_   { -1 };
  int          signal_num_    { -1 };

  SrcList      src_list_;
  DestList     dest_list_;
};

#endif
