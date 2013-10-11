#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/sha256.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

int main(int argc, char **argv)
{
  char *sha;
  sha = sha256StringFromString("The quick brown fox jumps over the lazy dog", false);
  printf("Test 0 : %s\n", PASSFAIL(strcmp(sha, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592") == 0));
  free(sha);

  sha = sha256StringFromString("The quick brown fox jumps over the lazy dog.", false);
  printf("Test 1 : %s\n", PASSFAIL(strcmp(sha, "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c") == 0));
  free(sha);

  sha = sha256StringFromString("", false);
  printf("Test 2 : %s\n", PASSFAIL(strcmp(sha, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0));
  free(sha);

  FILE *fin = fopen("./../source/sha256.c", "rb");
  if(fin)
  {
    sha = sha256StringFromFile(fin, false);
    printf("Test 3 : %s\n", PASSFAIL(strcmp(sha, "4b83c582b8e91264d9c6d24aa4857ebefa60aebd91b45d28a2a2c6ca5556825d") == 0));
    fclose(fin); free(sha);
  }

  return 0;
}

