#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clib.h>

int main(int argc, char **argv)
{
  u32 decodedLength;
  char *sRaw    = "Hello World in base64!";
  char *sBase64 = base64EncodeArr((u8 *)sRaw, strlen(sRaw));
  char *sReRaw  = (char *)base64DecodeStr(sBase64, &decodedLength);

  //sReRaw is really a byte array, and NOT zero-terminated
  //overwriting, realloc()ing, or something else is required to use it as a cstr
  sReRaw[decodedLength-1] = 0x00;

  printf("%s\n", sRaw   );
  printf("%s\n", sBase64);
  printf("%s\n", sReRaw ); //overwrote '!'

  free(sBase64);
  free(sReRaw);
  return 0;
}
