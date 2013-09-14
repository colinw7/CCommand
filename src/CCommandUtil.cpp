#include "CCommandI.h"
#include <cstdio>
#include <cstring>

void
CCommandUtil::
outputMsg(const char *format, ...)
{
  static FILE *output_fp;

  if (output_fp == NULL)
    output_fp = fopen(".msg.txt", "w");

  if (output_fp == NULL)
    return;

  va_list args;

  va_start(args, format);

  vfprintf(output_fp, format, args);

  va_end(args);

  fflush(output_fp);
}
