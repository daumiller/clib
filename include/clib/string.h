// string.h
//==============================================================================
#ifndef CLIB_STRING_HEADER
#define CLIB_STRING_HEADER
//==============================================================================
#include <stdarg.h>
#include <clib/types.h>
//==============================================================================

#ifndef __APPLE__
char *strdup(char *str);
char *strndup(char *str, u32 count);
#endif

//------------------------------------------------------------------------------

typedef struct
{
  char *data;
  u32   length;
} string;

string *stringCreate();
string *stringFromCharacter(char c, u32 count);
string *stringFromCString(char *str);
string *stringFromNCString(char *str, u32 length);
string *stringDuplicate(string *str);
void stringFree(string **str);

void stringAppendCharacter(string *str, char c, u32 count);
void stringAppendCString(string *str, char *cstr);
void stringAppendNCString(string *str, char *cstr, u32 length);
void stringAppendString(string *into, string *from);

void stringAssignCharacter(string *str, char c, u32 count);
void stringAssignCString(string *str, char *cstr);
void stringAssignNCString(string *str, char *cstr, u32 length);
void stringAssignString(string *into, string *from);

void stringTrim(string *str, bool leading, bool trailing);
void stringClear(string *str);
void stringLowerCase(string *str);
void stringUpperCase(string *str);
string *stringSubstring(string *str, u32 index, u32 length);
string **stringSplit(string *str, char *separators, u8 sepCount, u32 *tokenCount);
void stringPrintF(string *str, u32 buffSize, const char *format, ...);
void stringVPrintF(string *str, u32 buffSize, const char *format, va_list arg);
char *stringToCString(string *str);

//------------------------------------------------------------------------------

struct stringBuilderComponent
{
  void     *data;
  u32       length;
  struct stringBuilderComponent *next;
};

typedef struct
{
  struct stringBuilderComponent *origin;
  struct stringBuilderComponent *final;
  u32    length;
} stringBuilder;

stringBuilder *stringBuilderCreate();
void stringBuilderFree(stringBuilder **sb);
void stringBuilderAppendCharacter(stringBuilder *sb, char c, u32 count);
void stringBuilderAppendCString(stringBuilder *sb, char *str);
void stringBuilderAppendNCString(stringBuilder *sb, char *str, u32 length);
void stringBuilderAppendString(stringBuilder *sb, string *str);
char *stringBuilderToCString(stringBuilder *sb);
string *stringBuilderToString(stringBuilder *sb);

//==============================================================================
#endif //CLIB_STRING_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

