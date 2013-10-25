#include <stdio.h>
#include <stdlib.h>
#include <clib/string.h>

int main(int argc, char **argv)
{
  // line one
  char *lineOne = "line 1\n";

  // line two, created from printf *string*
  string *lineTwo = stringCreate();
  stringPrintF(lineTwo, 1024, "line %d%c", 2, '\n');

  // combine lines in a stringBuilder
  stringBuilder *sb = stringBuilderCreate();
  stringBuilderAppendCString  (sb, lineOne);
  stringBuilderAppendString   (sb, lineTwo);
  stringBuilderAppendCString  (sb, "line 3");
  stringBuilderAppendCharacter(sb, '\n', 2);  // two newlines

  // flatten stringBuilder to a c string, output results
  char *combined = stringBuilderToCString(sb);
  printf("%s", combined);

  //cleanup
  free(combined);
  stringBuilderFree(&sb);
  stringFree(&lineTwo);

  return 0;
}
