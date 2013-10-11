// sha256.c
//==============================================================================
#include <string.h>
#include <malloc.h>
#include <clib/sha256.h>
//==============================================================================
static void sha256Feed_Helper(sha256 *sha);
static char *sha256String_Helper(u8 *bytes, bool upperCase);
//==============================================================================
#define BYTETOU32(z,y) { ((u8 *)&y)[3]=(z)[0]; ((u8 *)&y)[2]=(z)[1]; ((u8 *)&y)[1]=(z)[2]; ((u8 *)&y)[0]=(z)[3]; }
#define U32TOBYTE(y,z) { (z)[0]=((u8 *)&y)[3]; (z)[1]=((u8 *)&y)[2]; (z)[2]=((u8 *)&y)[1]; (z)[3]=((u8 *)&y)[0]; }
#define U64TOBYTE(y,z) { U32TOBYTE(((u32 *)&y)[1],z); U32TOBYTE(((u32 *)&y)[0],z+4); }
#define ROTR(x,y) ((x >> y) | (x << (32 - y)))
//==============================================================================
static u32 sha256RoundConstant[] = {
  0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
  0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
  0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
  0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
  0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
  0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
  0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
  0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};
//==============================================================================

sha256 *sha256Begin()
{
  sha256 *sha = (sha256 *)malloc(sizeof(sha256));
  sha->length    = 0;
  sha->chunkSize = 0;
  sha->hash[0] = 0x6A09E667;
  sha->hash[1] = 0xBB67AE85;
  sha->hash[2] = 0x3C6EF372;
  sha->hash[3] = 0xA54FF53A;
  sha->hash[4] = 0x510E527F;
  sha->hash[5] = 0x9B05688C;
  sha->hash[6] = 0x1F83D9AB;
  sha->hash[7] = 0x5BE0CD19;
  return sha;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sha256Feed(sha256 *sha, void *data, u32 length)
{
  u32 needed;
  while(length)
  {
    needed = (64 - sha->chunkSize);
    if(length < needed)
    {
      memcpy(sha->chunk + sha->chunkSize, data, length);
      sha->chunkSize += length;
      return;
    }
    memcpy(sha->chunk + sha->chunkSize, data, needed);
    sha256Feed_Helper(sha);
    data   += needed;
    length -= needed;
    sha->chunkSize = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sha256Complete(sha256 *sha, u8 *hash)
{
  u8 padding[56]; memset(padding, 0x00, 56);
  u64 preLength = (sha->length + sha->chunkSize) << 3;
  u8 preLengthBytes[8]; U64TOBYTE(preLength, preLengthBytes);

  u8 one = 0x80;
  sha256Feed(sha, &one, 1);
  if((64 - sha->chunkSize) < 8) sha256Feed(sha, padding, 64 - sha->chunkSize);
  if((sha->chunkSize < 56))     sha256Feed(sha, padding, 56 - sha->chunkSize);
  sha256Feed(sha, preLengthBytes, 8);

  U32TOBYTE(sha->hash[0], hash   );
  U32TOBYTE(sha->hash[1], hash+ 4);
  U32TOBYTE(sha->hash[2], hash+ 8);
  U32TOBYTE(sha->hash[3], hash+12);
  U32TOBYTE(sha->hash[4], hash+16);
  U32TOBYTE(sha->hash[5], hash+20);
  U32TOBYTE(sha->hash[6], hash+24);
  U32TOBYTE(sha->hash[7], hash+28);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sha256Free(sha256 **sha)
{
  free(*sha);
  *sha = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void sha256Feed_Helper(sha256 *sha)
{
  sha->length += 64;

  u32 s0, s1, working[64];
  u32 a,b,c,d,e,f,g,h;

  BYTETOU32(sha->chunk   , working[ 0]); BYTETOU32(sha->chunk+ 4, working[ 1]);
  BYTETOU32(sha->chunk+ 8, working[ 2]); BYTETOU32(sha->chunk+12, working[ 3]);
  BYTETOU32(sha->chunk+16, working[ 4]); BYTETOU32(sha->chunk+20, working[ 5]);
  BYTETOU32(sha->chunk+24, working[ 6]); BYTETOU32(sha->chunk+28, working[ 7]);
  BYTETOU32(sha->chunk+32, working[ 8]); BYTETOU32(sha->chunk+36, working[ 9]);
  BYTETOU32(sha->chunk+40, working[10]); BYTETOU32(sha->chunk+44, working[11]);
  BYTETOU32(sha->chunk+48, working[12]); BYTETOU32(sha->chunk+52, working[13]);
  BYTETOU32(sha->chunk+56, working[14]); BYTETOU32(sha->chunk+60, working[15]);

  a=sha->hash[0]; b=sha->hash[1]; c=sha->hash[2]; d=sha->hash[3];
  e=sha->hash[4]; f=sha->hash[5]; g=sha->hash[6]; h=sha->hash[7];

  for(u32 i=16; i<64; i++)
  {
    s0 = ROTR(working[i-15], 7) ^ ROTR(working[i-15],18) ^ (working[i-15]>> 3);
    s1 = ROTR(working[i- 2],17) ^ ROTR(working[i- 2],19) ^ (working[i- 2]>>10);
    working[i] = working[i-16] + s0 + working[i-7] + s1;
  }

  for(u32 i=0; i<64; i++)
  {
    s0  = (ROTR(e,6) ^ ROTR(e,11) ^ ROTR(e,25)) +
          ((e & f) ^ ((~e) & g))                +
          h + sha256RoundConstant[i] + working[i];
    s1  = (ROTR(a,2) ^ ROTR(a,13) ^ ROTR(a,22)) +
          ((a & b) ^ (a & c) ^ (b & c));

    h=g; g=f; f=e; e=d+s0;
    d=c; c=b; b=a; a=s0+s1;
  }

  sha->hash[0]+=a; sha->hash[1]+=b; sha->hash[2]+=c; sha->hash[3]+=d;
  sha->hash[4]+=e; sha->hash[5]+=f; sha->hash[6]+=g; sha->hash[7]+=h;
}

//------------------------------------------------------------------------------

u8 *sha256FromData(void *data, u32 length)
{
  u8 *hash = (u8 *)malloc(32);
  sha256 *sha = sha256Begin();
  sha256Feed(sha, data, length);
  sha256Complete(sha, hash);
  sha256Free(&sha);
  return hash;
}

u8 *sha256FromString(char *str)
{
  u32 length = (u32)strlen(str);
  u8 *hash = (u8 *)malloc(32);
  sha256 *sha = sha256Begin();
  sha256Feed(sha, str, length);
  sha256Complete(sha, hash);
  sha256Free(&sha);
  return hash;
}

u8 *sha256FromFile(FILE *fin)
{
  char *buff = (char *)malloc(4096);
  u32 read; u8 *hash = (u8 *)malloc(32);
  sha256 *sha = sha256Begin();
  while(!feof(fin))
  {
    read = fread(buff, 1, 4096, fin);
    if(read) sha256Feed(sha, buff, read);
  }
  free(buff);
  sha256Complete(sha, hash);
  sha256Free(&sha);
  return hash;
}

//------------------------------------------------------------------------------

char *sha256StringFromData(void *data, u32 length, bool upperCase)
{
  u8 *bytes = sha256FromData(data, length);
  return sha256String_Helper(bytes, upperCase);
}

char *sha256StringFromString(char *str, bool upperCase)
{
  u8 *bytes = sha256FromString(str);
  return sha256String_Helper(bytes, upperCase);
}

char *sha256StringFromFile(FILE *fin, bool upperCase)
{
  u8 *bytes = sha256FromFile(fin);
  return sha256String_Helper(bytes, upperCase);
}

static char *sha256String_Helper(u8 *bytes, bool upperCase)
{
  u8 bChar;
  char *hash = (char *)malloc(65);
  char base = (upperCase ? 'A' : 'a');
  for(u32 i=0; i<32; i++)
  {
    bChar =  bytes[i]     & 0xF; if(bChar<0xA) hash[(i<<1)|1]='0'+bChar; else hash[(i<<1)|1]=base+(bChar-0xA);
    bChar = (bytes[i]>>4) & 0xF; if(bChar<0xA) hash[ i<<1   ]='0'+bChar; else hash[ i<<1   ]=base+(bChar-0xA);
  }
  free(bytes);
  hash[64] = 0x00;
  return hash;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

