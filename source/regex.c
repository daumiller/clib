// regex.c
//==============================================================================
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include PCREHEADER
#include <clib/types.h>
#include <clib/string.h>
#include <clib/regex.h>
//==============================================================================
void  regexReplace_HelpA(regex *regex, char *str, char *sub, char **pre, char **mid, char **post);
char *regexReplace_HelpB(regexMatch *set, char *sub);
char *regexReplace_HelpC(char *origin, char *replace, char *with);
//==============================================================================

regex *regexCreate(char *expr, u32 flags, char **err)
{
  bool compile = true;
  if(flags & REGEX_NO_COMPILE)
  {
    flags &= ~REGEX_NO_COMPILE;
    compile = false;
  }
  if(flags & REGEX_NO_UTF8)
    flags &= ~REGEX_NO_UTF8;
  else
    flags |= (PCRE_UTF8 | PCRE_NO_UTF8_CHECK);

  int errCode, errPos; const char *errStr;
  pcre *pcre = pcre_compile2((const char *)expr, flags, &errCode, &errStr, &errPos, NULL);
  if(pcre == NULL)
  {
    if(err)
    {
      char *errTemplate = "Regex Compilation Error: (%d) %s\n    %s\n    ";
      char *outErrStr = (char *)malloc(strlen(errTemplate) + strlen(errStr) + strlen(expr) + strlen(expr) + 33);
      int pos = sprintf(outErrStr, errTemplate, errCode, errStr, expr);
      for(int i=0; i<errPos; i++) pos += sprintf(outErrStr+pos, " ");
      sprintf(outErrStr+pos, "^");
      *err = outErrStr;
    }
    return NULL;
  }

  pcre_extra *study = NULL;
  if(compile)
  {
    study = pcre_study(pcre, PCRE_STUDY_JIT_COMPILE, &errStr);
    if(errStr != NULL) if(study != NULL) { pcre_free_study(study); study = NULL; }
  }

  if(err) *err = NULL;
  regex *r = (regex *)malloc(sizeof(regex));
  r->pcre  = (void *)pcre;
  r->study = (void *)study;
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void regexFree(regex **regex)
{
  if((*regex)->study) pcre_free_study((pcre_extra *)(*regex)->study);
  pcre_free((*regex)->pcre);
  free(*regex);
  *regex = NULL;
}

//------------------------------------------------------------------------------

bool regexIsMatch(regex *regex, char *str)
{
  int vectors[72]; //non-static local allows thread safety
  int status = pcre_exec((const pcre *)regex->pcre, (const pcre_extra *)regex->study, str, strlen(str), 0,0, vectors, 72);
  return (status > -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

regexMatch *regexExecute(regex *regex, char *str)
{
  int status = 0, maxVectors = 16, *vectors = NULL, len = strlen(str);
  while((status == 0) && (maxVectors < 4096))
  {
    maxVectors <<= 1; //start with 32 (*3) vectors
    if(vectors) free(vectors);
    vectors = (int *)malloc(sizeof(int) * (maxVectors * 3));
    if(vectors == NULL) return NULL;
    status = pcre_exec((const pcre *)regex->pcre, (const pcre_extra *)regex->study, str, len, 0,0, vectors, (maxVectors * 3));
  }
  if(status <  0) { free(vectors); return NULL; } //no match
  if(status == 0) { free(vectors); return NULL; } //give up matching (after 4K*3)

  int length, count = status;
  regexSub *matchArr = (regexSub *)malloc(sizeof(regexSub) * count);
  for(int i=0; i<count; i++)
  {
    status = i<<1;
    matchArr[i].position = vectors[status];
    length = vectors[status|1] - matchArr[i].position;
    matchArr[i].string = clstrdupn(str+matchArr[i].position, length);
  }
  free(vectors);

  regexMatch *result = (regexMatch *)malloc(sizeof(regexMatch));
  result->count = count;
  result->sub   = matchArr;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *regexReplace(regex *regex, char *str, char *sub, bool global)
{
  int len;
  char *pre, *mid, *post, *tmpA, *tmpB, *result=NULL;

  regexReplace_HelpA(regex, str, sub, &pre, &mid, &post);
  while(post)
  {
    len = 0;
    if(result) len += strlen(result);
    if(pre   ) len += strlen(pre   );
    if(mid   ) len += strlen(mid   );
    tmpA = (char *)malloc(len + 1);
    
    tmpB = tmpA;
    if(result) { tmpB += sprintf(tmpB, "%s", result); free(result); result = NULL; }
    if(pre   ) { tmpB += sprintf(tmpB, "%s", pre   ); free(pre   ); pre    = NULL; }
    if(mid   ) { tmpB += sprintf(tmpB, "%s", mid   ); free(mid   ); mid    = NULL; }

    result = tmpA;
    tmpB = tmpA = NULL;
    if(!global) break;

    tmpA = post;
    post = NULL;
    regexReplace_HelpA(regex, tmpA, sub, &pre, &mid, &post);
    free(tmpA);
  }

  len = 0;
  if(result) len += strlen(result);
  if(pre   ) len += strlen(pre   );
  if(mid   ) len += strlen(mid   );
  if(post  ) len += strlen(post  );
  tmpA = (char *)malloc(len + 1);

  tmpB = tmpA;
  if(result) { tmpB += sprintf(tmpB, "%s", result); free(result); result = NULL; }
  if(pre   ) { tmpB += sprintf(tmpB, "%s", pre   ); free(pre   ); pre    = NULL; }
  if(mid   ) { tmpB += sprintf(tmpB, "%s", mid   ); free(mid   ); mid    = NULL; }
  if(post  ) { tmpB += sprintf(tmpB, "%s", post  ); free(post  ); post   = NULL; }

  return tmpA;
}

void regexReplace_HelpA(regex *regex, char *str, char *sub, char **pre, char **mid, char **post)
{
  int i;
  *pre = *mid = *post = NULL;
  regexMatch *set = regexExecute(regex, str);
  if(set == NULL) { (*pre)=clstrdup(str); return; }

  i = strlen(set->sub[0].string);
  if(set->sub[0].position > 0)                 *pre  = clstrdupn(str, set->sub[0].position);
  if((set->sub[0].position + i) < strlen(str)) *post = clstrdup(str + set->sub[0].position + i);

  *mid = regexReplace_HelpB(set, sub);
  regexMatchFree(&set);
  return;
}

char *regexReplace_HelpB(regexMatch *set, char *sub)
{
  char *working = clstrdup(sub), *tmp;
  char ref[32];
  for(u32 i=1; i<set->count; i++)
  {
    sprintf(ref, "$%d", i);
    tmp = regexReplace_HelpC(working, ref, set->sub[i].string);
    free(working);
    working = tmp;
  }
  return working;
}

char *regexReplace_HelpC(char *origin, char *replace, char *with)
{
  char *processed = NULL, *pre, *post, *tmpA, *tmpB, *working = clstrdup(origin);
  int lenRep = strlen(replace), lenWith = strlen(with), lenProc=0, len;

  tmpA = strstr(working, replace);
  int idx = (tmpA == NULL) ? -1 : (tmpA - working);
  while(idx > -1)
  {
    pre=post=NULL; len=0;
    if(processed) len += lenProc;
    if(idx > 0) { pre = clstrdupn(working, idx); len += strlen(pre); }
    len += lenWith;
    tmpB = tmpA = (char *)malloc(len + 1);
    if(processed) { tmpB += sprintf(tmpB, "%s", processed); free(processed); }
    if(pre      ) { tmpB += sprintf(tmpB, "%s", pre      ); free(pre      ); }
                            sprintf(tmpB, "%s", with     );
    processed = tmpA;
    lenProc = len;

    len = strlen(working);
    if((idx + lenRep) < (len+1))
    {
      tmpA = clstrdup(working + idx + lenRep);
      free(working);
      working = tmpA;
      tmpA = strstr(working, replace);
      idx = (tmpA == NULL) ? -1 : (working - tmpA);
    }
    else
      idx = -1;
  }

  if(working)
  {
    tmpA = (char *)malloc(lenProc + strlen(working) + 1);
    tmpB = tmpA;
    if(processed) { tmpB += sprintf(tmpB, "%s", processed); free(processed); }
    sprintf(tmpB, "%s", working);
    free(working);
    processed = tmpA;
  }
  return processed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void regexMatchFree(regexMatch **m)
{
  for(u32 i=0; i<(*m)->count; i++)
    free((*m)->sub[i].string);
  free((*m)->sub);
  free(*m);
  *m = NULL;  
}

//------------------------------------------------------------------------------

char *regexEscape(char *str)
{
  int len = strlen(str);
  char *buff = (char *)malloc((len<<1) | 1);
  int a = 0;
  for(int i=0; i<len; i++,a++)
  {
    switch(str[i])
    {
      case '{': case '}':
      case '[': case ']':
      case '(': case ')':
      case '+': case '*': case '?':
      case '^': case '$':
      case '.': case '\\':
        buff[a] = '\\';
        a++;
    }
    buff[a] = str[i];
  }
  buff[a] = 0x00;
  return buff;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

