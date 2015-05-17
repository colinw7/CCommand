#include <CCommand.h>
#include <CFile.h>
#include <CStrUtil.h>
#include <CReadLine.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>

#define NONE     0
#define COMMAND  1
#define INPUT    2
#define OUTPUT   3
#define ERROR    4
#define PIPE     5
#define PIPE_ERR 6
#define ARGS     7

struct Command {
  std::string              name;
  std::vector<std::string> args;
  int                 type;
};

typedef std::vector<std::string>::iterator pstring;

void process_line(const std::string &line);

int
main(int argc, char **argv)
{
  std::string filename;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-')
      std::cerr << "Invalid option " << argv[i] << std::endl;
    else
      filename = argv[i];
  }

  if (filename != "") {
    CFile file(filename);

    std::vector<std::string> lines;

    file.toLines(lines);

    uint num_lines = lines.size();

    for (uint i = 0; i < num_lines; ++i)
      process_line(lines[i]);
  }
  else {
    CReadLine readline;

    readline.setPrompt("> ");

    for (;;) {
      std::string line = readline.readLine();

      if (! readline.eof())
        process_line(line);
    }
  }

  return 0;
}

void
process_line(const std::string &line)
{
  std::string line1 = CStrUtil::stripSpaces(line);

  if (line1[0] == '#')
    return;

  //------

  std::vector<std::string> words;

  CStrUtil::toWords(line1, words);

  uint num_words = words.size();

  if (num_words == 0)
    return;

  //------

  if (words[0] == "exit")
    exit(0);

  //------

  std::cout << "--- " << line1 << " ---" << std::endl;

  //------

  std::vector<CCommand *> pcommands;
  std::vector<int>        ptypes;

  int state = COMMAND;

  std::vector<std::string> args;
  std::vector<std::string> stdin__files;
  std::vector<std::string> stdout_files;
  std::vector<std::string> stderr_files;

  std::string name;

  for (int i = 0; i < words.size(); i++) {
    if      (words[i] == "<")
      state = INPUT;
    else if (words[i] == ">")
      state = OUTPUT;
    else if (words[i] == ">&")
      state = ERROR;
    else if (words[i] == "|")
      state = PIPE;
    else if (words[i] == "|&")
      state = PIPE_ERR;
    else {
      if      (state == COMMAND) {
        name = words[i];

        state = ARGS;
      }
      else if (state == INPUT) {
        stdin__files.push_back(words[i]);

        state = ARGS;
      }
      else if (state == OUTPUT) {
        stdout_files.push_back(words[i]);

        state = ARGS;
      }
      else if (state == ERROR) {
        stderr_files.push_back(words[i]);

        state = ARGS;
      }
      else if (state == PIPE || state == PIPE_ERR) {
        CCommand *pcommand = new CCommand(name, name, args);

        args.empty();

        pstring p;

        for (p = stdin__files.begin(); p != stdin__files.end(); ++p)
          pcommand->addFileSrc(*p);

        for (p = stdout_files.begin(); p != stdout_files.end(); ++p)
          pcommand->addFileDest(*p, 1);

        for (p = stderr_files.begin(); p != stderr_files.end(); ++p)
          pcommand->addFileDest(*p, 2);

        stdin__files.empty();
        stdout_files.empty();
        stderr_files.empty();

        pcommands.push_back(pcommand);
        ptypes   .push_back(state);

        //------

        name = words[i];

        state = ARGS;
      }
      else
        args.push_back(words[i]);
    }
  }

  //-----

  CCommand command(name, name, args);

  pstring p;

  for (p = stdin__files.begin(); p != stdin__files.end(); ++p)
    command.addFileSrc(*p);

  for (p = stdout_files.begin(); p != stdout_files.end(); ++p)
    command.addFileDest(*p, 1);

  for (p = stderr_files.begin(); p != stderr_files.end(); ++p)
    command.addFileDest(*p, 2);

  //-----

  uint num_pcommands = pcommands.size();

  if (num_pcommands > 0) {
    pcommands[0]->addPipeDest(1);

    if (ptypes[0] == PIPE_ERR)
      pcommands[0]->addPipeDest(2);

    for (uint i = 1; i < num_pcommands; ++i) {
      pcommands[i]->addPipeSrc();

      pcommands[i]->addPipeDest(1);

      if (ptypes[i] == PIPE_ERR)
        pcommands[i]->addPipeDest(2);
    }

    command.addPipeSrc();
  }

  try {
    for (uint i = 0; i < num_pcommands; ++i)
      pcommands[i]->start();

    command.start();

    for (uint i = 0; i < num_pcommands; ++i) {
      pcommands[i]->wait();

      // while (pcommand->getState() == CCommand::COMMAND_RUNNING)
      //   sleep(1);
    }

    command.wait();

    // while (command.getState() == CCommand::COMMAND_RUNNING)
    //   sleep(1);
  }
  catch (const std::string message) {
    fprintf(stderr, "%s\n", (char *) message.c_str());
  }
  catch (const char *message) {
    fprintf(stderr, "%s\n", message);
  }
  catch (...) {
    fprintf(stderr, "Failed\n");
  }
}
