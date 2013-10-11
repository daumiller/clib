// base64.c
//==============================================================================
#include <string.h>
#include <malloc.h>
#include <clib/types.h>
#include <clib/base64.h>
//==============================================================================
static const char *_idx64  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//============================================================================== 
static void encodeTriplet(u8 *cin, u32 length, char *cout);
static void decodeQuartet(char *cin, u8 *cout, u32 *size);
static u8   revIndex64(char cin, u32 *size);
//============================================================================== 

char *base64EncodeArr(u8 *data, u32 length)
{
  u32 mod = (length % 3);
  u32 size = ((length - mod) / 3) * 4;
  if(mod > 0) size += 4;
  char *cout = (char *)malloc(size + 1);
  for(u32 i=0,o=0,x; i<length; i+=3,o+=4)
  {
    x = length-i; if(x>3) x=3;
    encodeTriplet(((u8 *)data)+i, x, cout+o);
  }
  cout[size] = 0x00;
  return cout;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void base64EncodeFile(FILE *fin, FILE *fout)
{
  u8   *buffIn  = (u8   *)malloc(3072);
  char *buffOut = (char *)malloc(4096);
  u32 lenIn, lenOut, x;

  while(!feof(fin))
  {
    lenOut = 0;
    lenIn  = fread(buffIn, 1, 3072, fin);
    for(u32 i=0,o=0; i<lenIn; i+=3,o+=4)
    {
      x = lenIn - i; if(x>3) x=3;
      encodeTriplet(buffIn+i, x, buffOut+o);
      lenOut += 4;
    }
    x=0; while(x<lenOut) x += fwrite(buffOut+x, 1, lenOut-x, fout);
  }
}

//------------------------------------------------------------------------------

u8 *base64DecodeStr(char *data, u32 *size)
{
  u32 length = strlen(data);
  *size = (length>>2) * 3;
  if(data[length-1] == '=') (*size)--;
  if(data[length-2] == '=') (*size)--;
  u8 *cout = (u8 *)malloc(*size);
  for(u32 i=0,o=0,x; i<length; i+=4,o+=3)
    decodeQuartet(data+i, cout+o, &x);
  return cout;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void base64DecodeFile(FILE *fin, FILE *fout)
{
  char *buffIn  = (char *)malloc(4096);
  u8   *buffOut = (u8   *)malloc(3072);
  u32 lenIn, lenOut, x;

  while(!feof(fin))
  {
    lenOut = 0;
    lenIn  = fread(buffIn, 1, 4096, fin);
    for(u32 i=0,o=0; i<lenIn; i+=4,o+=3)
    {
      decodeQuartet(buffIn+i, buffOut+o, &x);
      lenOut += x;
    }
    x=0; while(x<lenOut) x += fwrite(buffOut+x, 1, lenOut-x, fout);
  }
}

//==============================================================================
static void encodeTriplet(u8 *cin, u32 length, char *cout)
{
  u32 encVal = 0x000000;
                 encVal |= ((u32)cin[0]) << 16;
  if(length > 1) encVal |= ((u32)cin[1]) <<  8;
  if(length > 2) encVal |= ((u32)cin[2])      ;
                 cout[0] = _idx64[(encVal >> 18) & 0x3F];
                 cout[1] = _idx64[(encVal >> 12) & 0x3F];
  if(length > 1) cout[2] = _idx64[(encVal >>  6) & 0x3F]; else cout[2] = '=';
  if(length > 2) cout[3] = _idx64[(encVal      ) & 0x3F]; else cout[3] = '=';

  u32 test; u8 ctest[3]; bool good=true;
  decodeQuartet(cout, ctest, &test);
                 if(cin[0] != ctest[0]) good = false;
  if(length > 1) if(cin[1] != ctest[1]) good = false;
  if(length > 2) if(cin[2] != ctest[2]) good = false;
  if(good == false)
  {
    if(length == 1)
      printf("{ %02X } -> { %c %c %c %c } -> { %02X }\n",
        (u32)cin[0],
        cout[0], cout[1], cout[2], cout[3],
        (u32)ctest[0]);
    else if(length == 2)
      printf("{ %02X %02X } -> { %c %c %c %c } -> { %02X %02X }\n",
        (u32)cin[0], (u32)cin[1],
        cout[0], cout[1], cout[2], cout[3],
        (u32)ctest[0], (u32)ctest[1]);
    else
      printf("{ %02X %02X %02X } -> { %c %c %c %c } -> { %02X %02X %02X }\n",
        (u32)cin[0], (u32)cin[1], (u32)cin[2],
        cout[0], cout[1], cout[2], cout[3],
        (u32)ctest[0], (u32)ctest[1], (u32)ctest[2]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void decodeQuartet(char *cin, u8 *cout, u32 *size)
{
  *size = 3;
  u8 a = revIndex64(cin[0], NULL);
  u8 b = revIndex64(cin[1], NULL);
  u8 c = revIndex64(cin[2], size);
  u8 d = revIndex64(cin[3], size);
  u32 decVal = (a<<18) | (b<<12) | (c<<6) | d;
                cout[0] = (decVal>>16) & 0xFF;
  if(*size > 1) cout[1] = (decVal>> 8) & 0xFF;
  if(*size > 2) cout[2] = (decVal    ) & 0xFF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static u8 revIndex64(char cin, u32 *size)
{
  if(cin >= 'A') if(cin <= 'Z') return ( 0 + (cin - 'A'));
  if(cin >= 'a') if(cin <= 'z') return (26 + (cin - 'a'));
  if(cin >= '0') if(cin <= '9') return (52 + (cin - '0'));
  if(cin == '+') return 62;
  if(cin == '/') return 63;
  if(size != NULL) if(cin == '=') (*size)--;
  if(cin != '=') printf("revIndex64(%c) : Out of Index\n", cin);
  return 0;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

