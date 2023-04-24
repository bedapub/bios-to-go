#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"

#define STARTUP_MSG "Hello World Example Program"
#define PROG_VERSION "DEV"
#define AUTHOR_MAIL "roland.schmucki@roche.com"


void usagef (int level)
{
  romsg ("\n"
         "Program: %s \n\n"
         "Version: %s \n\n"
         "Notes:   %s \n\n"
         "Input parameters: none\n\n"
         "\n\n"
         "Report bugs and feedback to %s"
         "\n\n",
         arg_getProgName (),PROG_VERSION,STARTUP_MSG,AUTHOR_MAIL);
}



int main (int argc,char *argv[])
{

  printf("\n\nHello World!\n\n");

  return 0;
}
