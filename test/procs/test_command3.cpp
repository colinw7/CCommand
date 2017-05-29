#include "std_c++.h"
#include "termios.h"
#include "CCommand/CCommand.h"

typedef vector<string>::iterator pstring;

void
command_proc(const vector<string> &args, CCommand::CallbackData data) {
  cout << "command_proc" << endl;

  for (int i = 0; i < args.size(); i++)
    cout << args[i] << endl;
}

int
main(int argc, char **argv)
{
  try {
    vector<string> args;

    for (int i = 1; i < argc; i++)
      args.push_back(string(argv[i]));

    CCommand command(argv[0], command_proc, NULL, args);

    command.addFileDest("args.txt");

    command.start();
  }
  catch (...) {
    cout << "Error" << endl;
  }

  return 0;
}
