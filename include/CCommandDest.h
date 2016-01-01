#ifndef CCommandDest_H
#define CCommandDest_H

#include <string>
#include <cstdio>

class CCommand;

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

#endif
