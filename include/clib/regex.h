// regex.h
//==============================================================================
#ifndef CLIB_REGEX_HEADER
#define CLIB_REGEX_HEADER
//==============================================================================
#include <clib/types.h>
//==============================================================================
#define REGEX_DEFAULT          0x00000000
#define REGEX_CASE_INSENSITIVE 0x00000001
#define REGEX_MULTILINE        0x00000002
#define REGEX_DOT_ALL          0x00000004
#define REGEX_DOLLAR_EOF       0x00000020
#define REGEX_UNGREEDY         0x00000200
#define REGEX_NEWLINE_CR       0x00100000
#define REGEX_NEWLINE_LF       0x00200000
#define REGEX_NEWLINE_CRLF     0x00300000
#define REGEX_NEWLINE_ANY      0x00400000
#define REGEX_NEWLINE_ANYCRLF  0x00500000
#define REGEX_JAVASCRIPT       0x02000000
#define REGEX_NO_UTF8          0x20000000
#define REGEX_NO_COMPILE       0x40000000
//==============================================================================

typedef struct
{
  void *pcre;
  void *study;
} regex;

typedef struct
{
  char *string;
  u32   position;
} regexSub;

typedef struct
{
  u32       count;
  regexSub *sub;
} regexMatch;

//------------------------------------------------------------------------------

regex *regexCreate(char *expr, u32 flags, char **err);
void   regexFree  (regex **regex);

bool        regexIsMatch(regex *regex, char *str);
regexMatch *regexExecute(regex *regex, char *str);
char       *regexReplace(regex *regex, char *str, char *sub, bool global);

void regexMatchFree(regexMatch **m);
char *regexEscape(char *str);

//==============================================================================
#endif //CLIB_REGEX_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

