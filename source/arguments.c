// arguments.c
//==============================================================================
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
# include <malloc.h>
#endif
#include <clib/arguments.h>
#include <clib/string.h>
//==============================================================================
struct argMatcher
{
  char        *dash;
  char        *dashDash;
  argumentType type;
  bool         required;
  bool         found;
  void        *destination;
  u32          longLen;
};
//==============================================================================
static struct argMatcher *createArgMatcher(char *dash, char *dashDash, void *adest, argumentType type);
static bool testArgMatch(arguments *args, struct argMatcher **matcher, u32 matcherCount, int *argIdx, int argc, char **argv, stringBuilder *sb, char *errBuff);
static bool processMatchedArg(arguments *args, struct argMatcher *matcher, char *str, int *argIdx, int argc, char **argv, stringBuilder *sb, char *errBuff);
//==============================================================================

void argumentsFree(arguments **args)
{
  if((*args)->argc > 0) free((*args)->argv);
  if((*args)->message)  free((*args)->message);
  free(*args);
  *args = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

arguments *argumentsParse(int argc, char **argv, bool allowExtra, u32 count, ...)
{
  va_list vl;
  va_start(vl, count);
  arguments *args = argumentsParseV(argc, argv, allowExtra, count, vl);
  va_end(vl);
  return args;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

arguments *argumentsParseV(int argc, char **argv, bool allowExtra, u32 count, va_list vl)
{
  arguments *args = (arguments *)calloc(1,sizeof(arguments));

  if(count == 0)
  {
    args->argc = argc-1;
    if(args->argc > 0)
    {
      args->argv    = argv+1;
      if(allowExtra == false)
      {
        args->code    = ARGCODE_EXTRA;
        args->message = strdup("No arguments supported.");
      }
    }
    return args;
  }

  void *adest;
  argumentType atype;
  char *dash, *dashDash;
  struct argMatcher **matchers = (struct argMatcher **)malloc(sizeof(struct argMatcher *) * count);

  //parse matchers from parameters
  for(u32 i=0; i<count; i++)
  {
    dash = va_arg(vl, char *);
    dashDash = NULL;
    if(dash[0] == '|')
    {
      dashDash = dash+1;
      dash = NULL;
    }
    else if(dash[1] == '|')
    {
      dashDash = (dash[2] == 0x00) ? NULL : dash+2;
    }

    atype  = va_arg(vl, argumentType);
    adest  = va_arg(vl, void *);
    switch(atype & 0x0F)
    {
      case ARGTYPE_FLAG    : *((bool * )adest) = (bool)va_arg(vl, int   ); break;
      case ARGTYPE_CSTRING : *((char **)adest) =       va_arg(vl, char *); break;
      case ARGTYPE_I32     : *((i32  * )adest) =       va_arg(vl, i32   ); break;
      case ARGTYPE_F32     : *((f32  * )adest) = (f32) va_arg(vl, double); break;
      default : printf("argumentsParse: Invalid ARGTYPE (%u)\n", atype & 0xF); exit(-1);
    }
    matchers[i] = createArgMatcher(dash, dashDash, adest, atype);
  }

  //parse options from arguments
  stringBuilder *sb = stringBuilderCreate();
  args->argv = (char **)calloc(argc-1, sizeof(char *));
  char *errBuff = (char *)malloc(4096);
  for(int i=1; i<argc; i++)
  {
    if(testArgMatch(args, matchers, count, &i, argc, argv, sb, errBuff) == false)
    {
      args->argv[args->argc] = argv[i];
      args->argc++;
      if(allowExtra == false)
      {
        args->code |= ARGCODE_EXTRA;
        snprintf(errBuff, 4096, "Unknown argument \"%s\".", argv[i]);
        stringBuilderAppendCString(sb, errBuff);
      }
    }
  }
  
  //make sure all required parameters were met
  for(u32 i=0; i<count; i++)
    if(matchers[i]->required == true)
      if(matchers[i]->found == false)
      {
        if(matchers[i]->dash && matchers[i]->dashDash)
          snprintf(errBuff, 4096, "Missing required argument \"%c|%s\".", matchers[i]->dash[0], matchers[i]->dashDash);
        else if(matchers[i]->dash)
          snprintf(errBuff, 4096, "Missing required argument \"%c\".", matchers[i]->dash[0]);
        else if(matchers[i]->dashDash)
          snprintf(errBuff, 4096, "Missing required argument \"%s\".", matchers[i]->dashDash);
        else
          snprintf(errBuff, 4096, "A supposedly-required argument is missing; but was not programmed properly...");
        stringBuilderAppendCString(sb, errBuff);
        args->code |= ARGCODE_MISSING;
      }

  stringBuilderSeparateCString(sb, "\n");
  args->message = stringBuilderToCString(sb);
  stringBuilderFree(&sb);

  free(errBuff);
  free(matchers);
  return args;
}

//------------------------------------------------------------------------------

static struct argMatcher *createArgMatcher(char *dash, char *dashDash, void *adest, argumentType type)
{
  struct argMatcher *matcher = (struct argMatcher *)malloc(sizeof(struct argMatcher));
  matcher->dash        = dash;
  matcher->dashDash    = dashDash;
  matcher->required    = ((type & 0xF0) == ARGTYPE_REQUIRED);
  matcher->found       = false;
  matcher->type        = (type & 0x0F);
  matcher->longLen     = dashDash ? strlen(dashDash) : 0;
  matcher->destination = adest;
  return matcher;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static bool testArgMatch(arguments *args, struct argMatcher **matcher, u32 matcherCount, int *argIdx, int argc, char **argv, stringBuilder *sb, char *errBuff)
{
  char *str = argv[*argIdx];
  if((str == NULL) || (str[0] == 0x00) || (str[0] != '-') || (str[1] == 0x00)) return false;

  if(str[1] != '-') //single-dash parameter
  {
    if((str[2] != 0x00) && (str[2] != '=')) return false; //long argument with single hyphen
    for(u32 i=0; i<matcherCount; i++)
      if(matcher[i]->dash != NULL)
        if(matcher[i]->dash[0] == str[1])
          return processMatchedArg(args, matcher[i], str+2, argIdx, argc, argv, sb, errBuff);
  }
  else //double-dash parameter
  {
    if(str[2] == 0x00) return false; // "--"
    u32 strLen = strlen(str) - 2;
    for(u32 i=0; i<matcherCount; i++)
      if(matcher[i]->dashDash != NULL)
        if(strLen >= matcher[i]->longLen)
          if((str[matcher[i]->longLen+2] == 0x00) || (str[matcher[i]->longLen+2] == '='))
            if(strncmp(matcher[i]->dashDash, str+2, matcher[i]->longLen) == 0)
              return processMatchedArg(args, matcher[i], str+2+matcher[i]->longLen, argIdx, argc, argv, sb, errBuff);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static bool processMatchedArg(arguments *args, struct argMatcher *matcher, char *str, int *argIdx, int argc, char **argv, stringBuilder *sb, char *errBuff)
{
  if(matcher->type == ARGTYPE_FLAG)
  {
    if(str[0] == '=')
    {
      char *desc = matcher->dashDash; if(desc==NULL) desc=matcher->dash;
      snprintf(errBuff, 4096,  "Value \"%s\" passed to flag parameter %s.", str+1, desc);
      stringBuilderAppendCString(sb, errBuff);
      args->code |= ARGCODE_INVALID;
      return true; //match was true, though processing failed
    }
    *((bool *)(matcher->destination)) = true;
    matcher->found = true;
    return true;
  }

  //get value
  if(str[0] == '=')
    str++;
  else
  {
    if((*argIdx) < argc)
    {
      (*argIdx)++;
      str = argv[*argIdx];
    }
    else
    {
      //if an value option is specified, that value is required
      char *desc = matcher->dashDash; if(desc==NULL) desc=matcher->dash;
      snprintf(errBuff, 4096, "Missing value for non-flag parameter %s.", desc);
      stringBuilderAppendCString(sb, errBuff);
      args->code |= ARGCODE_INVALID;
      return true; //match was true, processing failed
    }
  }

  switch(matcher->type)
  {
    case ARGTYPE_CSTRING :
      *((char **)(matcher->destination)) = str;
    break;

    case ARGTYPE_I32 :
      *((i32 *)(matcher->destination)) = atoi(str);
    break;

    case ARGTYPE_F32 :
      *((f32 *)(matcher->destination)) = (float)atof(str);
    break;

    default: break; //disable warning; type check taken care of in main function
  }
  matcher->found = true;
  return true;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

