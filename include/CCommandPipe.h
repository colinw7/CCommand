#ifndef CCommandPipe_H
#define CCommandPipe_H

#include <list>
#include <string>

class CCommand;

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

#endif
