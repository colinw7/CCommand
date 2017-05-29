#ifndef CCommandStringDest_H
#define CCommandStringDest_H

#include <CCommandDest.h>

class CCommandPipe;

class CCommandStringDest : public CCommandDest {
 public:
  CCommandStringDest(CCommand *command, std::string &str, int dest_fd=1);

 ~CCommandStringDest();

  void initParent();
  void initChild();
  void term();

  void process();

  CCommandPipe *getPipe() const { return pipe_; }
  int           getFd  () const { return dest_fd_; }

 private:
  std::string  &str_;
  int           dest_fd_ { 0 };
  CCommandPipe *pipe_    { nullptr };
  int           fd_      { 0 };
  std::string   filename_;
};

#endif
