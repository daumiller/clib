// arguments.c
//==============================================================================
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __APPLE__
# include <malloc.h>
#endif
#include <clib/arguments.h>
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
static bool testArgMatch(char *arg, struct argMatcher **matcher, u32 matcherCount, int *argIdx, int argCount, char **args, bool *bail);
static bool processMatchedArg(struct argMatcher *matcher, char *arg, int *argIdx, int argCount, char **args, bool *bail);
//==============================================================================

bool argumentsParse(int *argc, char ***argv, bool reject, u32 count, ...)
{
  va_list vl;
  va_start(vl, count);
  return argumentsParseV(argc, argv, reject, count, vl);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool argumentsParseV(int *argc, char ***argv, bool reject, u32 count, va_list vl)
{
  if(count == 0)
  {
    if(reject == false) return true;
    printf("No arguments supported.\n");
    return false;
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
      default : printf("argumentParse: Invalid ARGTYPE (%u)\n", atype&0xF); exit(-1);
    }
    matchers[i] = createArgMatcher(dash, dashDash, adest, atype);
  }
  va_end(vl);

  //parse options from arguments
  bool bail = false;
  int argcOut; char **argvOut = (char **)calloc((*argc),sizeof(char *));
  argcOut = 1; argvOut[0] = (*argv)[0];
  for(int i=1; i<(*argc); i++)
  {
    if(testArgMatch((*argv)[i], matchers, count, &i, *argc, *argv, &bail) == false)
    {
      if(reject == true)
      {
        printf("Unknown argument \"%s\".\n", (*argv)[i]);
        bail = true;
      }
      else
      {
        argvOut[argcOut] = (*argv)[i];
        argcOut++;
      }
    }
  }
  if(bail)
  {
    free(argvOut);
    free(matchers);
    return false;
  }

  //make sure all required parameters were met
  bool anyMissing = false;
  for(u32 i=0; i<count; i++)
    if(matchers[i]->required == true)
      if(matchers[i]->found == false)
      {
        if(matchers[i]->dash && matchers[i]->dashDash)
          printf("Missing required argument \"%c|%s\".\n", matchers[i]->dash[0], matchers[i]->dashDash);
        else if(matchers[i]->dash)
          printf("Missing required argument \"%c\".\n", matchers[i]->dash[0]);
        else if(matchers[i]->dashDash)
          printf("Missing required argument \"%s\".\n", matchers[i]->dashDash);
        else
          printf("A supposedly-required argument is missing; but was not programmed properly...\n");
        anyMissing = true;
      }
  if(anyMissing)
  {
    free(argvOut);
    free(matchers);
    return false;
  }

  *argc = argcOut;
  *argv = argvOut;
  return true;
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

static bool testArgMatch(char *arg, struct argMatcher **matcher, u32 matcherCount, int *argIdx, int argCount, char **args, bool *bail)
{
  if((arg == NULL) || (arg[0] == 0x00) || (arg[0] != '-') || (arg[1] == 0x00)) return false;

  if(arg[1] != '-') //single-dash parameter
  {
    if((arg[2] != 0x00) && (arg[2] != '=')) return false; //long argument with single hyphen
    for(u32 i=0; i<matcherCount; i++)
      if(matcher[i]->dash != NULL)
        if(matcher[i]->dash[0] == arg[1])
          return processMatchedArg(matcher[i], arg+2, argIdx, argCount, args, bail);
  }
  else //double-dash parameter
  {
    if(arg[2] == 0x00) return false; // "--"
    u32 argLen = strlen(arg) - 2;
    for(u32 i=0; i<matcherCount; i++)
      if(matcher[i]->dashDash != NULL)
        if(argLen >= matcher[i]->longLen)
          if((arg[matcher[i]->longLen+2] == 0x00) || (arg[matcher[i]->longLen+2] == '='))
            if(strncmp(matcher[i]->dashDash, arg+2, matcher[i]->longLen) == 0)
              return processMatchedArg(matcher[i], arg+2+matcher[i]->longLen, argIdx, argCount, args, bail);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static bool processMatchedArg(struct argMatcher *matcher, char *arg, int *argIdx, int argCount, char **args, bool *bail)
{
  if(matcher->type == ARGTYPE_FLAG)
  {
    if(arg[0] == '=')
    {
      char *desc = matcher->dashDash; if(desc==NULL) desc=matcher->dash;
      printf("Value \"%s\" passed to flag parameter %s.\n", arg, desc);
      *bail = true;
      return true;
    }
    *((bool *)(matcher->destination)) = true;
    matcher->found = true;
    return true;
  }

  //get value
  if(arg[0] == '=')
    arg++;
  else
  {
    if((*argIdx) < argCount)
    {
      (*argIdx)++;
      arg = args[*argIdx];
    }
    else
    {
      //if an value option is specified, that value is required
      *bail = true;
      char *desc = matcher->dashDash; if(desc==NULL) desc=matcher->dash;
      printf("Missing value for non-flag parameter %s.\n", desc);
      return true;
    }
  }

  switch(matcher->type)
  {
    case ARGTYPE_CSTRING :
      *((char **)(matcher->destination)) = arg;
    break;

    case ARGTYPE_I32 :
      *((i32 *)(matcher->destination)) = atoi(arg);
    break;

    case ARGTYPE_F32 :
      *((f32 *)(matcher->destination)) = (float)atof(arg);
    break;

    default: break; //disable warning; type check taken care of in main function
  }
  matcher->found = true;
  return true;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

