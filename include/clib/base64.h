// base64.h
//==============================================================================
#ifndef CLIB_BASE64_HEADER
#define CLIB_BASE64_HEADER
//==============================================================================
#include <stdio.h>
#include <clib/types.h>
//==============================================================================

char *base64EncodeArr(u8   *data, u32 length);
u8   *base64DecodeStr(char *data, u32 *size);

void  base64EncodeFile(FILE *fin, FILE *fout);
void  base64DecodeFile(FILE *fin, FILE *fout);

//==============================================================================
#endif //CLIB_BASE64_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
