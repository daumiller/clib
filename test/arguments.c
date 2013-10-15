#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/arguments.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

void verify(arguments *args, argumentCode code, bool testVerbose, float testFloat, char *testString, int testArgc);

i32   test;
bool  verbose;
f32   argFloat;
char *argString;
int   extra;
int   globArgc;

int main(int argc, char **argv)
{
  arguments *args = argumentsParse(argc, argv, false, 4,
                                   "t|test"   , ARGTYPE_I32    |ARGTYPE_REQUIRED, &test     , 0         ,
                                   "v|verbose", ARGTYPE_FLAG   |ARGTYPE_OPTIONAL, &verbose  , false     ,
                                   "f|float"  , ARGTYPE_F32    |ARGTYPE_OPTIONAL, &argFloat , 3.14159f  ,
                                   "s|string" , ARGTYPE_CSTRING|ARGTYPE_OPTIONAL, &argString, "eleventy");

  switch(test)
  {
    case  0: verify(args, ARGCODE_MISSING, false, 3.14159, "eleventy"        , 0); break;
    case  1: verify(args, ARGCODE_OKAY   , false, 3.14159, "eleventy"        , 0); break;
    case  2: verify(args, ARGCODE_OKAY   , false, 3.14159, "eleventy"        , 0); break;
    case  3: verify(args, ARGCODE_OKAY   , true , 3.14159, "eleventy"        , 0); break;
    case  4: verify(args, ARGCODE_OKAY   , false, 49.0   , "eleventy"        , 0); break;
    case  5: verify(args, ARGCODE_OKAY   , false, 64.32  , "eleventy"        , 0); break;
    case  6: verify(args, ARGCODE_EXTRA  , false, 3.14159, "eleventy"        , 1); break;
    case  7: verify(args, ARGCODE_OKAY   , false, 12.0   , "test7"           , 0); break;
    case  8: verify(args, ARGCODE_OKAY   , false, 3.14159, "another test"    , 0); break;
    case  9: verify(args, ARGCODE_OKAY   , true , 45.0   , "test number nine", 0); break;
    case 10: verify(args, ARGCODE_INVALID, false, 3.14159, "eleventy"        , 0); break;
    case 11: verify(args, ARGCODE_EXTRA  , false, 3.14159, "eleventy"        , 3); break;
    case 12: verify(args, ARGCODE_OKAY   , true , 3.14159, "--query"         , 0); break;
    case 13: verify(args, ARGCODE_EXTRA  , false, 12.0   , "eleventy"        , 1); break;
  }

  argumentsFree(&args);
  return 0;
}

void verify(arguments *args, argumentCode code, bool testVerbose, float testFloat, char *testString, int testArgc)
{
  bool success = true;  
  if(args->code != code) success = false;
  if(testVerbose != verbose) success = false;
  if(fabs(testFloat - argFloat) > 0.0001) success = false;
  if(testString && argString && strcmp(testString,argString)) success = false;
  if(((testString == NULL) || (argString == NULL)) && (testString != argString)) success = false;
  if(testArgc != args->argc) success = false;
  printf("Test %2d : %s", test, PASSFAIL(success));
  if(args->code != ARGCODE_OKAY) printf("\n%s", args->message);
  printf("\n");
}

