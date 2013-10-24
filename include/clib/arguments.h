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

typedef enum
{
  ARGCODE_OKAY    = 0x00,
  ARGCODE_EXTRA   = 0x01,
  ARGCODE_MISSING = 0x02,
  ARGCODE_INVALID = 0x04
} argumentCode;

typedef struct
{
  argumentCode code;
  char        *message;
  int          argc;
  char       **argv;
} arguments;

arguments *argumentsParse (int argc, char **argv, bool allowExtra, u32 count, ...);
arguments *argumentsParseV(int argc, char **argv, bool allowExtra, u32 count, va_list arg);
void argumentsFree(arguments **args);

//==============================================================================
#endif //CLIB_ARGUMENTS_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

