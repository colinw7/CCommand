#ifndef COMMAND_H
#define COMMAND_H

#include <CSingleton.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <list>
#include <map>

typedef std::vector<std::string> StringVectorT;
typedef std::vector<int>         IntVectorT;

class CCommand;
class CCommandPipe;
class CCommandSrc;
class CCommandDest;
class CCommandPipeDest;

#define CCommandMgrInst CCommandMgr::getInstancePtr()

class CCommand {
 public:
  typedef std::list<CCommandSrc *>  SrcList;
  typedef std::list<CCommandDest *> DestList;

  typedef void  *CallbackData;
  typedef void (*CallbackProc)(const StringVectorT &args, CallbackData data);

  enum State {
    NO_STATE,
    IDLE_STATE,
    RUNNING_STATE,
    EXITED_STATE,
    SIGNALLED_STATE,
    STOPPED_STATE
  };

 public:
  CCommand(const std::string &cmdStr, bool do_fork=true);

  CCommand(const std::string &name, const std::string &path,
           const StringVectorT &args=StringVectorT(), bool do_fork=true);

  CCommand(const std::string &name, CallbackProc proc, CallbackData data,
           const StringVectorT &args=StringVectorT(), bool do_fork=false);

  virtual ~CCommand();

  const std::string &getName() const { return name_; }
  const std::string &getPath() const { return path_; }

  uint getId() const { return id_; }
  void setId(uint id) { id_ = id; }

  bool getDoFork() const { return do_fork_; }
  void setDoFork(bool do_fork) { do_fork_ = do_fork; }

  CallbackProc getCallbackProc() const { return callback_proc_; }
  CallbackData getCallbackData() const { return callback_data_; }

  const StringVectorT &getArgs   ()      const { return args_; }
  int                  getNumArgs()      const { return args_.size(); }
  const std::string   &getArg    (int i) const { return args_[i]; }

  void addArg(const std::string &arg) { args_.push_back(arg); }

  pid_t getPid () const { return pid_ ; }

  bool isChild() const { return child_; }

  State getState() const { return state_; }
  bool  isState (State state) const { return (state_ == state); }

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
  void init(const StringVectorT &args);

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
  std::string   name_;
  std::string   path_;
  uint          id_;
  bool          do_fork_;
  CallbackProc  callback_proc_;
  CallbackData  callback_data_;
  StringVectorT args_;
  pid_t         pid_;
  pid_t         pgid_;
  bool          group_leader_;
  uint          group_id_;
  bool          child_;

  State         state_;
  int           return_code_;
  int           signal_num_;

  SrcList       src_list_;
  DestList      dest_list_;
};

class CCommandMgr : public CSingleton<CCommandMgr> {
 public:
  typedef std::map<uint,CCommand *> CommandMap;
  typedef std::list<CCommand *>     CommandList;


 public:
  CCommandMgr();

  void addCommand   (CCommand *);
  void deleteCommand(CCommand *);

  CCommand *getCommand(uint id) const;

  CommandMap::iterator commandsBegin() { return command_map_.begin(); }
  CommandMap::iterator commandsEnd  () { return command_map_.end  (); }

  CCommandPipeDest *getPipeDest() const {
    return pipe_dest_;
  }

  void setPipeDest(CCommandPipeDest *pipe_dest) {
    pipe_dest_ = pipe_dest;
  }

  std::string getLastError() const { return last_error_; }

  void setThrowOnError(bool flag) { throwOnError_ = flag; }

  bool getDebug() const { return debug_; }

  void setDebug(bool debug) { debug_ = debug; }

  bool execCommand(const std::string &cmd);

  CCommand *lookup(pid_t pid);

  CommandList getCommands();
  CommandList getCommands(CCommand::State state);

  void throwError(const std::string &msg);

 private:
  CommandMap        command_map_;
  CCommandPipeDest *pipe_dest_;
  std::string       last_error_;
  uint              last_id_;
  bool              throwOnError_;
  bool              debug_;
};

class CCommandPipe {
 public:
  typedef std::list<CCommandPipe *> PipeList;

 public:
  CCommandPipe(CCommand *command);
 ~CCommandPipe();

  void setSrc (CCommand *src ) { src_  = src ; }
  void setDest(CCommand *dest) { dest_ = dest; }

  int getInput () const { return fd_[0]; }
  int getOutput() const { return fd_[1]; }

  int closeInput();
  int closeOutput();

  static void deleteOthers(CCommand *command);

 private:
  void throwError(const std::string &msg);

 private:
  CCommand *command_;
  int       fd_[2];
  CCommand *src_;
  CCommand *dest_;

  static PipeList pipes_;
};

class CCommandSrc {
 public:
  CCommandSrc(CCommand *command);
  CCommandSrc(CCommand *command, FILE *fp);

  virtual ~CCommandSrc();

  CCommand *getCommand() const { return command_; }

  virtual void initParent() = 0;
  virtual void initChild() = 0;
  virtual void term() = 0;
  virtual void process() { }

 protected:
  void throwError(const std::string &msg);

 protected:
  CCommand *command_;
  int      fd_;
  int      save_stdin_;
};

class CCommandFileSrc : public CCommandSrc {
 public:
  CCommandFileSrc(CCommand *command, const std::string &file);
  CCommandFileSrc(CCommand *command, FILE *fp);

 ~CCommandFileSrc();

  void initParent();
  void initChild();
  void term();

 private:
  std::string *file_;
};

class CCommandPipeSrc : public CCommandSrc {
 public:
  CCommandPipeSrc(CCommand *command);

 ~CCommandPipeSrc();

  void setDest(CCommandPipeDest *pipe_dest);

  void setPipe(CCommandPipe *pipe);

  void initParent();
  void initChild();
  void term();

  CCommandPipe *getPipe() const { return pipe_; }

 private:
  CCommandPipeDest *pipe_dest_;
  CCommandPipe     *pipe_;
};

class CCommandStringSrc : public CCommandSrc {
 public:
  CCommandStringSrc(CCommand *command, const std::string &str);

 ~CCommandStringSrc();

  void initParent();
  void initChild();
  void term();

  void process();

  CCommandPipe *getPipe() const { return pipe_; }

 private:
  std::string   str_;
  CCommandPipe *pipe_;
};

class CCommandDest {
 public:
  CCommandDest(CCommand *command);
  CCommandDest(CCommand *command, FILE *fp);

  virtual ~CCommandDest();

  CCommand *getCommand() const { return command_; }

  virtual void initParent() = 0;
  virtual void initChild() = 0;
  virtual void term() = 0;
  virtual void process() { }

 protected:
  void throwError(const std::string &msg);

 protected:
  CCommand *command_;
  int       fd_;
  int       save_fd_;
};

class CCommandFileDest : public CCommandDest {
 public:
  CCommandFileDest(CCommand *command, const std::string &file, int fd);
  CCommandFileDest(CCommand *command, FILE *fp, int fd);

 ~CCommandFileDest();

  int getFd() const { return dest_fd_; }

  void setOverwrite(bool overwrite) { overwrite_ = overwrite; }
  void setAppend(bool append) { append_ = append; }

  void initParent();
  void initChild();
  void term();

 private:
  std::string *file_;
  int          dest_fd_;
  bool         overwrite_;
  bool         append_;
};

class CCommandStringDest : public CCommandDest {
 public:
  CCommandStringDest(CCommand *command, std::string &str, int dest_fd=1);

 ~CCommandStringDest();

  void initParent();
  void initChild();
  void term();

  void process();

  CCommandPipe *getPipe() const { return pipe_; }
  int          getFd  () const { return dest_fd_; }

 private:
  std::string  &str_;
  int           dest_fd_;
  CCommandPipe *pipe_;
  int           fd_;
  std::string   filename_;
};

class CCommandPipeDest : public CCommandDest {
 public:
  CCommandPipeDest(CCommand *command);

 ~CCommandPipeDest();

  void setSrc(CCommandPipeSrc *pipe_src);

  void setPipe(CCommandPipe *pipe);

  void addFd(int fd);

  void initParent();
  void initChild();
  void term();

  CCommandPipe *getPipe() const { return pipe_; }

 private:
  CCommandPipeSrc *pipe_src_;
  CCommandPipe    *pipe_;
  IntVectorT       dest_fds_;
  IntVectorT       save_fds_;
};

class CCommandUtil {
 public:
  static void outputMsg(const char *format, ...);
};

#endif
