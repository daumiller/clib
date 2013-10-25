#include <stdio.h>
#include <string.h>
#include <clib/sha256.h>

int main(int argc, char **argv)
{
  char *data = "Hello SHA-256 hashing world!";
  char *hash = sha256StringFromString(data, false);
  char *expc = "5350853de3cf4b8b8eac9b989a65839519b187cbeda158b2154a42f27f448345";

  printf("Input    : %s\n", data);
  printf("Output   : %s\n", hash);
  printf("Expected : %s\n", expc);
  printf("Correct  : %s\n", (strcmp(hash,expc) == 0) ? "yes" : "no");

  return 0;
}
