#ifndef CCommandSrc_H
#define CCommandSrc_H

#include <string>
#include <cstdio>

class CCommand;

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
  CCommand *command_    { nullptr };
  int       fd_         { -1 };
  int       save_stdin_ { -1 };
};

#endif
