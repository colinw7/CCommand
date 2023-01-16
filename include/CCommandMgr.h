#ifndef CCommandMgr_H
#define CCommandMgr_H

#include <CCommand.h>
#include <CSingleton.h>
#include <map>
#include <list>

class CCommandPipeDest;

#define CCommandMgrInst CCommandMgr::getInstancePtr()

class CCommandMgr : public CSingleton<CCommandMgr> {
 public:
  typedef std::map<uint, CCommand *> CommandMap;
  typedef std::list<CCommand *>      CommandList;

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
  CCommandPipeDest *pipe_dest_    { nullptr };
  std::string       last_error_;
  uint              last_id_      { 0 };
  bool              throwOnError_ { false };
  bool              debug_        { false };
};

#endif
