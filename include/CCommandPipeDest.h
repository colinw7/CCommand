#ifndef CCommandPipeDest_H
#define CCommandPipeDest_H

#include <CCommandDest.h>
#include <vector>

class CCommandPipeSrc;
class CCommandPipe;

class CCommandPipeDest : public CCommandDest {
 public:
  typedef std::vector<int> IntVectorT;

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
  CCommandPipeSrc *pipe_src_ { nullptr };
  CCommandPipe    *pipe_     { nullptr };
  IntVectorT       dest_fds_;
  IntVectorT       save_fds_;
};

#endif
