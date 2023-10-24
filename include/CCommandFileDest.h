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

  void initParent() override;
  void initChild() override;
  void term() override;

 private:
  std::string *file_      { nullptr };
  int          dest_fd_   { 0 };
  bool         overwrite_ { true };
  bool         append_    { false };
};

#endif
