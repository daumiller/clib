#include <stdio.h>
#include <stdlib.h>
#include <clib/regex.h>

int main(int argc, char **argv)
{
  char *input = "Hello World";
  printf("Original String : %s\n", input);

  char *err;
  // create a regular expression to match to strings
  regex *rx = regexCreate("([^\\s]+) ([^\\s]+)", REGEX_DEFAULT, &err);
  if(err)
  {
    printf("Regular expression error : %s\n", err);
    free(err);
    return -1;
  }

  // see if we match
  printf("Is a Match      : %s\n", regexIsMatch(rx, input) ? "yes" : "no");

  // swap our words
  char *output = regexReplace(rx, input, "$2 $1", false);
  printf("Replaced String : %s\n", output);

  //cleanup
  free(output);
  regexFree(&rx);

  return 0;
}
