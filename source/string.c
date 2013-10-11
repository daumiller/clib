// string.c
//==============================================================================
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <clib/string.h>
#include <clib/list.h>
//==============================================================================
static void stringBuilderAppend_internal(stringBuilder *sb, char *data, u32 length);
//==============================================================================

char *strdup(char *str)
{
  char *result = (char *)malloc(strlen(str) +1);
  strcpy(result, str);
  return result;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char *strndup(char *str, u32 count)
{
  char *result = (char *)malloc(count + 1);
  strncpy(result, str, count);
  result[count] = 0x00;
  return result;
}

//==============================================================================

string *stringCreate()
{
  string *str = (string *)malloc(sizeof(string));
  str->data   = NULL;
  str->length = 0;
  return str;
}

string *stringFromCharacter(char c, u32 count)
{
  string *str = (string *)malloc(sizeof(string));
  if(count == 0) { str->data=NULL; str->length=0; return str; }
  str->data   = (char *)malloc(count);
  str->length = count;
  memset(str->data, (int)c, (size_t)count);
  return str;
}

string *stringFromCString(char *cstr)
{
  string *str = (string *)malloc(sizeof(string));
  if((cstr == NULL) || (cstr[0] == 0x00)) { str->data=NULL; str->length=0; return str; }
  u32 length = strlen(cstr);
  str->data   = (char *)malloc(length);
  str->length = length;
  memcpy(str->data, cstr, (size_t)length);
  return str;
}

string *stringFromNCString(char *cstr, u32 length)
{
  string *str = (string *)malloc(sizeof(string));
  if((cstr == NULL) || (length == 0)) { str->data=NULL; str->length=0; return str; }
  str->data   = (char *)malloc(length);
  str->length = length;
  memcpy(str->data, cstr, (size_t)length);
  return str;
}

string *stringDuplicate(string *str)
{
  string *rstr = (string *)malloc(sizeof(string));
  if(str->length == 0)
    rstr->data = NULL;
  else
  {
    rstr->data = (char *)malloc(str->length);
    memcpy(rstr->data, str->data, (size_t)str->length);
  }
  rstr->length = str->length;
  return rstr;
}

void stringFree(string **str)
{
  if((*str)->data) free((*str)->data);
  free(*str);
  *str = NULL;
}

//------------------------------------------------------------------------------

void stringAppendCharacter(string *str, char c, u32 count)
{
  u32 growTo = str->length + count;
  str->data = realloc(str->data, (size_t)growTo);
  memset(str->data + str->length, (int)c, (size_t)count);
  str->length = growTo;
}

void stringAppendCString(string *str, char *cstr)
{
  u32 cslen = strlen(cstr);
  u32 growTo = str->length + cslen;
  str->data = realloc(str->data, (size_t)growTo);
  memcpy(str->data + str->length, cstr, (size_t)cslen);
  str->length = growTo;
}

void stringAppendNCString(string *str, char *cstr, u32 length)
{
  u32 growTo = str->length + length;
  str->data = realloc(str->data, (size_t)growTo);
  memcpy(str->data + str->length, cstr, (size_t)length);
  str->length = growTo;
}

void stringAppendString(string *into, string *from)
{
  u32 growTo = into->length + from->length;
  into->data = realloc(into->data, (size_t)growTo);
  memcpy(into->data + into->length, from->data, (size_t)from->length);
  into->length = growTo;
}

//------------------------------------------------------------------------------

void stringAssignCharacter(string *str, char c, u32 count)
{
  str->data = realloc(str->data, count);
  memset(str->data, (int)c, (size_t)count);
  str->length = count;
}

void stringAssignCString(string *str, char *cstr)
{
  u32 cslen = strlen(cstr);
  str->data = realloc(str->data, (size_t)cslen);
  memcpy(str->data, cstr, (size_t)cslen);
  str->length = cslen;
}

void stringAssignNCString(string *str, char *cstr, u32 length)
{
  str->data = realloc(str->data, (size_t)length);
  memcpy(str->data, cstr, (size_t)length);
  str->length = length;
}

void stringAssignString(string *into, string *from)
{
  into->data = realloc(into->data, (size_t)from->length);
  memcpy(into->data, from->data, (size_t)from->length);
  into->length = from->length;
}

//------------------------------------------------------------------------------

void stringTrim(string *str, bool leading, bool trailing)
{
  if(str->length == 0) return;
  u32 start  = 0;
  u32 finish = str->length - 1;

  if(leading)
    while((start < finish) &&
      ((str->data[start]==' ') || (str->data[start]=='\t') || (str->data[start]=='\r') || (str->data[start]=='\n')))
        start++;
  if(trailing)
    while((finish > start) &&
      ((str->data[finish]==' ') || (str->data[finish]=='\t') || (str->data[finish]=='\r') || (str->data[finish]=='\n')))
        finish--;

  if(start == finish) { str->length = 0; free(str->data); str->data = NULL; return; }

  if(start == 0) if(finish == (str->length - 1)) return;
  u32 trimLen    = (finish - start) + 1;
  char *trimData = (char *)malloc(trimLen);
  memcpy(trimData, str->data + start, trimLen);
  free(str->data);
  str->data   = trimData;
  str->length = trimLen;
}

void stringClear(string *str)
{
  str->length = 0;
  free(str->data);
  str->data = NULL;
}

//these assume UTF8 composed solely of single-byte/ASCII characters
void stringLowerCase(string *str) { for(u32 i=0; i<str->length; i++) str->data[i] = tolower(str->data[i]); }
void stringUpperCase(string *str) { for(u32 i=0; i<str->length; i++) str->data[i] = toupper(str->data[i]); }

string **stringSplit(string *str, char *separators, u8 sepCount, u32 *tokenCount)
{
  list *split = listCreate();

  bool hit;
  char *data = str->data;
  for(u32 i=0,j=0; i<str->length; i++,j++)
  {
    hit = false;
    for(u32 c=0; c<sepCount; c++)
    {
      if(data[j] == separators[c])
      {
        hit = true;
        break;
      }
    }
    if(hit)
    {
      listAppend(split, stringFromNCString(data, j));
      data += j + 1;
      j = -1;
    }
  }
  listAppend(split, stringFromNCString(data, str->length - (data - str->data)));

  string **arr = (string **)listToArray(split, tokenCount);
  listFree(&split);
  return arr;
}

string *stringSubstring(string *str, u32 index, u32 length)
{
  if(length == 0) length = (str->length - index);
  return stringFromNCString(str->data + index, length);
}

void stringPrintF(string *str, u32 buffSize, const char *format, ...)
{
  va_list vl;
  va_start(vl, format);
  stringVPrintF(str, buffSize, format, vl);
}

void stringVPrintF(string *str, u32 buffSize, const char *format, va_list arg)
{
  char *buff = (char *)malloc(buffSize+1);
  u32 actual = vsprintf(buff, format, arg);
  buff = realloc(buff, actual);
  free(str->data);
  str->length = actual;
  str->data   = buff;
}

char *stringToCString(string *str)
{
  char *dest = (char *)malloc(str->length + 1);
  memcpy(dest, str->data, str->length);
  dest[str->length] = 0x00;
  return dest;
}

//==============================================================================

stringBuilder *stringBuilderCreate()
{
  stringBuilder *sb = (stringBuilder *)malloc(sizeof(stringBuilder));
  sb->origin = NULL;
  sb->final  = NULL;
  sb->length = 0;
  return sb;
}

void stringBuilderFree(stringBuilder **sb)
{
  struct stringBuilderComponent *tmp, *sbc = (*sb)->origin;
  while(sbc)
  {
    free(sbc->data);
    tmp = sbc->next;
    free(sbc);
    sbc = tmp;
  }
  free(*sb);
  *sb = NULL;
}

void stringBuilderAppendCharacter(stringBuilder *sb, char c, u32 count)
{
  char *buff = (char *)malloc(count);
  memset(buff, c, count);
  stringBuilderAppend_internal(sb, buff, count);
}

void stringBuilderAppendCString(stringBuilder *sb, char *str)
{
  u32 length = strlen(str);  
  char *buff = strndup(str, length);
  stringBuilderAppend_internal(sb, buff, length);
}

void stringBuilderAppendNCString(stringBuilder *sb, char *str, u32 length)
{
  char *buff = strndup(str, length);
  stringBuilderAppend_internal(sb, buff, length);
}

void stringBuilderAppendString(stringBuilder *sb, string *str)
{
  char *buff = (char *)malloc(str->length);
  memcpy(buff, str->data, str->length);
  stringBuilderAppend_internal(sb, buff, str->length);
}

char *stringBuilderToCString(stringBuilder *sb)
{
  char *buff = (char *)malloc(sb->length + 1);
  u32 wrote = 0;
  struct stringBuilderComponent *curr = sb->origin;
  while(curr)
  {
    memcpy(buff+wrote, curr->data, curr->length);
    wrote += curr->length;
    curr = curr->next; 
  }
  buff[sb->length] = 0x00;
  return buff;
}

string *stringBuilderToString(stringBuilder *sb)
{
  string *str = (string *)malloc(sizeof(string));
  str->data   = stringBuilderToCString(sb);
  str->length = sb->length;
  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void stringBuilderAppend_internal(stringBuilder *sb, char *data, u32 length)
{
  struct stringBuilderComponent *app = (struct stringBuilderComponent *)malloc(sizeof(struct stringBuilderComponent));
  app->data   = data;
  app->length = length;
  app->next   = NULL;

  if(sb->origin == NULL) sb->origin = app;
  if(sb->final != NULL) sb->final->next = app;
  sb->final   = app;
  sb->length += length;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

