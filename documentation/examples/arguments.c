#include <stdio.h>
#include <clib.h>

int main(int argc, char **argv)
{
  bool  argForce;  //user can 'force' whatever action we're performing (an optional flag)
  char *argOutput; //where we should write output (a required path string)
  i32   argCount;  //number of times we should process whatever (optional, default to 1)

  arguments *args = argumentsParse(argc, argv, false, 3,
                       "f|force" , ARGTYPE_FLAG   |ARGTYPE_OPTIONAL, &argForce , false,
                       "o|output", ARGTYPE_CSTRING|ARGTYPE_REQUIRED, &argOutput,  NULL,
                       "n|count" , ARGTYPE_I32    |ARGTYPE_OPTIONAL, &argCount ,     1);

  if(args->code == ARGCODE_OKAY)
  {
    //if(argForce || !alreadyDone)
    //  process something argCount times
    //  write output to argOutput
    argumentsFree(&args); //cleanup
    return 0;             //ran okay
  }
  else
  {
    printf("Error: %s\n", args->message); //tell user how they screwed up
    argumentsFree(&args);                 //cleanup
    return -1;                            //tell shell about the failure
  }
}
