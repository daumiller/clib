// sha256.h
//==============================================================================
#ifndef CLIB_BASE64_HEADER
#define CLIB_BASE64_HEADER
//==============================================================================
#include <stdio.h>
#include <clib/types.h>
//==============================================================================

typedef struct
{
  u64 length;
  u8  chunk[64];
  u32 chunkSize;
  u32 hash[8];
} sha256;

sha256 *sha256Begin();
void sha256Feed(sha256 *sha, void *data, u32 length);
void sha256Complete(sha256 *sha, u8 *hash);
void sha256Free(sha256 **sha);

u8 *sha256FromData  (void *data, u32 length);
u8 *sha256FromString(char *str);
u8 *sha256FromFile  (FILE *fin);

char *sha256StringFromData  (void *data, u32 length, bool upperCase);
char *sha256StringFromString(char *str, bool upperCase);
char *sha256StringFromFile  (FILE *fin, bool upperCase);

//==============================================================================
#endif //CLIB_BASE64_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

