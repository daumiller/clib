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

void verify(bool testPassed, bool testVerbose, float testFloat, char *testString, int testArgc);

bool  passed;
i32   test;
bool  verbose;
f32   argFloat;
char *argString;
int   extra;
int   globArgc;

int main(int argc, char **argv)
{
  passed = argumentsParse(&argc, &argv, false, 4,
                          "t|test"   , ARGTYPE_I32    |ARGTYPE_REQUIRED, &test     , 0         ,
                          "v|verbose", ARGTYPE_FLAG   |ARGTYPE_OPTIONAL, &verbose  , false     ,
                          "f|float"  , ARGTYPE_F32    |ARGTYPE_OPTIONAL, &argFloat , 3.14159f  ,
                          "s|string" , ARGTYPE_CSTRING|ARGTYPE_OPTIONAL, &argString, "eleventy");
  globArgc = argc;

  switch(test)
  {
    case  0: verify(false, false, 3.14159, "eleventy"        , 1); break;
    case  1: verify(true , false, 3.14159, "eleventy"        , 1); break;
    case  2: verify(true , false, 3.14159, "eleventy"        , 1); break;
    case  3: verify(true , true , 3.14159, "eleventy"        , 1); break;
    case  4: verify(true , false, 49.0   , "eleventy"        , 1); break;
    case  5: verify(true , false, 64.32  , "eleventy"        , 1); break;
    case  6: verify(true , false, 3.14159, "eleventy"        , 2); break;
    case  7: verify(true , false, 12.0   , "test7"           , 1); break;
    case  8: verify(true , false, 3.14159, "another test"    , 1); break;
    case  9: verify(true , true , 45.0   , "test number nine", 1); break;
    case 10: verify(false, false, 3.14159, "eleventy"        , 4); break;
    case 11: verify(true , false, 3.14159, "eleventy"        , 4); break;
    case 12: verify(true , true , 3.14159, "--query"         , 1); break;
    case 13: verify(true , false, 12.0   , "eleventy"        , 2); break;
  }

  if(passed) free(argv);
  return 0;
}

void verify(bool testPassed, bool testVerbose, float testFloat, char *testString, int testArgc)
{
  bool success = true;  
  if(testPassed != passed) success = false;
  if(testVerbose != verbose) success = false;
  if(fabs(testFloat - argFloat) > 0.0001) success = false;
  if(testString && argString && strcmp(testString,argString)) success = false;
  if(((testString == NULL) || (argString == NULL)) && (testString != argString)) success = false;
  if(testArgc != globArgc) success = false;
  printf("Test %d : %s\n", test, PASSFAIL(success));
}

