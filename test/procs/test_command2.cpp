#include <CCommand.h>
#include <termios.h>

int
main(int argc, char **argv)
{
  if (argc < 2)
    exit(1);

  std::string name = argv[1];

  std::vector<std::string> args;

  for (int i = 2; i < argc; i++)
    args.push_back(std::string(argv[i]));

  CCommand command(name, name, args);

  std::string source = "Hello World\n";

  command.addStringSrc(source);

  command.start();

  command.wait();

  return 0;
}
