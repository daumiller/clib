#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/regex.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

int main(int argc, char **argv)
{
  char *err;
  regex *rx = regexCreate("ab[0-9]c([0-9]+)", REGEX_DEFAULT, &err);
  if(err) { printf("%s\n", err); return -1; }

  bool isMatch;
  isMatch = regexIsMatch(rx, "ab3c9"  ); printf("Match   00 : %s\n", PASSFAIL(isMatch == true ));
  isMatch = regexIsMatch(rx, "abc9"   ); printf("Match   01 : %s\n", PASSFAIL(isMatch == false));
  isMatch = regexIsMatch(rx, "ab33c9" ); printf("Match   02 : %s\n", PASSFAIL(isMatch == false));
  isMatch = regexIsMatch(rx, "ab3c987"); printf("Match   03 : %s\n", PASSFAIL(isMatch == true ));

  regexMatch *match;
  match = regexExecute(rx, "ab3c9"  ); printf("Execute 00 : %s\n", PASSFAIL(match != NULL)); if(match) regexMatchFree(&match);
  match = regexExecute(rx, "abc9"   ); printf("Execute 01 : %s\n", PASSFAIL(match == NULL)); if(match) regexMatchFree(&match);
  match = regexExecute(rx, "ab33c9" ); printf("Execute 02 : %s\n", PASSFAIL(match == NULL)); if(match) regexMatchFree(&match);
  match = regexExecute(rx, "ab3c987"); printf("Execute 03 : %s\n", PASSFAIL(match != NULL)); if(match) regexMatchFree(&match);

  match = regexExecute(rx, "ab3c987"); printf("Execute 04 : %s\n", PASSFAIL(match->count == 2));
  printf("Execute 05 : %s\n", PASSFAIL(strcmp(match->sub[0].string,"ab3c987")==0));
  printf("Execute 06 : %s\n", PASSFAIL(strcmp(match->sub[1].string,"987")==0));
  regexMatchFree(&match);

  char *replaced;
  replaced = regexReplace(rx, "ab3c987"      , "--$1--", false); printf("Replace 00 : %s\n", PASSFAIL(strcmp(replaced,"--987--"      ) == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "ab3c987rst"   , "--$1--", false); printf("Replace 01 : %s\n", PASSFAIL(strcmp(replaced,"--987--rst"   ) == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "Qr3ab3c987"   , "--$1--", false); printf("Replace 02 : %s\n", PASSFAIL(strcmp(replaced,"Qr3--987--"   ) == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "Qr3ab3c987rst", "--$1--", false); printf("Replace 03 : %s\n", PASSFAIL(strcmp(replaced,"Qr3--987--rst") == 0)); if(replaced) free(replaced);
  regexFree(&rx);

  rx = regexCreate("([0-9])[0-9]*", REGEX_DEFAULT, &err);
  if(err) { printf("%s\n", err); return -1; }

  replaced = regexReplace(rx, "a1b21c321d4321", "[N]" , true ); printf("Replace 04 : %s\n", PASSFAIL(strcmp(replaced,"a[N]b[N]c[N]d[N]") == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "a1b21c321d4321", "[N]" , false); printf("Replace 05 : %s\n", PASSFAIL(strcmp(replaced,"a[N]b21c321d4321") == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "a1b21c321d4321", "[$1]", true ); printf("Replace 06 : %s\n", PASSFAIL(strcmp(replaced,"a[1]b[2]c[3]d[4]") == 0)); if(replaced) free(replaced);
  replaced = regexReplace(rx, "a1b21c321d4321", "[$1]", false); printf("Replace 07 : %s\n", PASSFAIL(strcmp(replaced,"a[1]b21c321d4321") == 0)); if(replaced) free(replaced);
  regexFree(&rx);

  char *escaped;
  escaped = regexEscape("ordinary string"  ); printf("Escape  00 : %s\n", PASSFAIL(strcmp(escaped,"ordinary string"        ) == 0)); if(escaped) free(escaped);
  escaped = regexEscape("ordinary $tring"  ); printf("Escape  01 : %s\n", PASSFAIL(strcmp(escaped,"ordinary \\$tring"      ) == 0)); if(escaped) free(escaped);
  escaped = regexEscape("ordinary [$tring]"); printf("Escape  02 : %s\n", PASSFAIL(strcmp(escaped,"ordinary \\[\\$tring\\]") == 0)); if(escaped) free(escaped);

  escaped = regexEscape("[$]");
  rx = regexCreate(escaped, REGEX_DEFAULT, &err);
  if(err) { printf("%s\n", err); return -1; }
  isMatch = regexIsMatch(rx, "ju[$]st so[$]me te[$]st"); printf("Combine 0a : %s\n", PASSFAIL(isMatch == true));
  replaced = regexReplace(rx, "ju[$]st so[$]me te[$]st", "", true); printf("Combine 0b : %s\n", PASSFAIL(strcmp(replaced,"just some test") == 0));
  free(replaced); free(escaped); regexFree(&rx);

  rx = regexCreate("[$]", REGEX_DEFAULT, &err);
  if(err) { printf("%s\n", err); return -1; }
  isMatch = regexIsMatch(rx, "ju[$]st so[$]me te[$]st"); printf("Combine 0c : %s\n", PASSFAIL(isMatch == true));
  replaced = regexReplace(rx, "ju[$]st so[$]me te[$]st", "", true); printf("Combine 0d : %s\n", PASSFAIL(strcmp(replaced,"just some test") != 0));
  free(replaced);
  regexFree(&rx);

  return 0;
}

