// arguments.h
//==============================================================================
#ifndef CLIB_ARGUMENTS_HEADER
#define CLIB_ARGUMENTS_HEADER
//==============================================================================
#include <stdarg.h>
#include <clib/types.h>
//==============================================================================

typedef enum
{
  ARGTYPE_FLAG     = 0x00,
  ARGTYPE_CSTRING  = 0x01,
  ARGTYPE_I32      = 0x02,
  ARGTYPE_F32      = 0x03,
  ARGTYPE_OPTIONAL = 0x00,
  ARGTYPE_REQUIRED = 0x10
} argumentType;

/*
bool  argForce;
char *argOutput;
i32   argCount;
bool ok = argumentsParse(&argc, &argc, true, 3,
                         "f|force" , ARGTYPE_FLAG    | ARGTYPE_OPTIONAL, &argForce , false,
                         "o|output", ARGTYPE_CSTRING | ARGTYPE_REQUIRED, &argOutput,  NULL,
                         "n|count" , ARGTYPE_I32     | ARGTYPE_OPTIONAL, &argCount ,     1);
if(ok) free(argv);
*/

bool argumentsParse (int *argc, char ***argv, bool reject, u32 count, ...);
bool argumentsParseV(int *argc, char ***argv, bool reject, u32 count, va_list arg);

//==============================================================================
#endif //CLIB_ARGUMENTS_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

