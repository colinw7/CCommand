#ifndef CCommandPipeSrc_H
#define CCommandPipeSrc_H

#include <CCommandSrc.h>

class CCommandPipeDest;
class CCommandPipe;

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

#endif
