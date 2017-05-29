#ifndef CCommandStringSrc_H
#define CCommandStringSrc_H

#include <CCommandSrc.h>

class CCommandPipe;

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
  CCommandPipe *pipe_ { nullptr };
};

#endif
