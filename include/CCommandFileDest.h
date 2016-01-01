#ifndef CCommandFileDest_H
#define CCommandFileDest_H

#include <CCommandDest.h>

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

#endif
