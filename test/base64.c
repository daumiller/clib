#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <clib/base64.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

int main(int argc, char **argv)
{
  char *testStr = "Man is distinguished, not only by his reason, but by this singular passion from "
                  "other animals, which is a lust of the mind, that by a perseverance of delight "
                  "in the continued and indefatigable generation of knowledge, exceeds the short "
                  "vehemence of any carnal pleasure.";
  u32 tlen = strlen(testStr);
  char *out1a = base64EncodeArr((void *)testStr, tlen);
  char *out1b = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
                "IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
                "dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
                "dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
                "ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";
  
  printf("Test 0a : %s\n", PASSFAIL(strcmp(out1a, out1b) == 0));

  u32 test1Len;
  char *out1c = (char *)base64DecodeStr(out1a, &test1Len);
  printf("Test 1a : %s\n", PASSFAIL(test1Len == tlen));
  printf("Test 1b : %s\n", PASSFAIL(strncmp(out1c, testStr, tlen) == 0));

  free(out1a);
  free(out1c);

#ifdef FILETEST
  FILE *fin, *fout;
  fin  = fopen("base64.png", "rb");
  fout = fopen("base64.b64", "wb");
  base64EncodeFile(fin, fout);
  fclose(fin); fclose(fout);
  fin  = fopen("base64.b64", "rb");
  fout = fopen("test64.png", "wb");
  base64DecodeFile(fin, fout);
  fclose(fin); fclose(fout);
#endif //FILETEST

  return 0;
}

