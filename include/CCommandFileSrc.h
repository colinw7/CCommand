#ifndef CCommandFileSrc_H
#define CCommandFileSrc_H

#include <CCommandSrc.h>

class CCommandFileSrc : public CCommandSrc {
 public:
  CCommandFileSrc(CCommand *command, const std::string &file);
  CCommandFileSrc(CCommand *command, FILE *fp);

 ~CCommandFileSrc();

  void initParent();
  void initChild();
  void term();

 private:
  std::string *file_ { nullptr };
};

#endif
