#include <stdio.h>
#include <clib/hash.h>

int main(int argc, char **argv)
{
  hash *table = hashCreate();

  // store some values
  hashSetValue(table, "blue"  , "ff0000ff");
  hashSetValue(table, "violet", "ffff00ff");
  hashSetValue(table, "cyan"  , "ff00ffff");

  // retreive values
  printf("blue   : %s\n", (char *)hashGetValue(table, "blue"  ));
  printf("violet : %s\n", (char *)hashGetValue(table, "violet"));
  printf("cyan   : %s\n", (char *)hashGetValue(table, "cyan"  ));

  // cleanup
  hashFree(&table);
  return 0;
}
