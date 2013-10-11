// types.h
//==============================================================================
#ifndef CLIB_TYPES_HEADER
#define CLIB_TYPES_HEADER
//==============================================================================
#include <stdint.h>
#include <stdbool.h>
//==============================================================================

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef  int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

typedef float    f32;
typedef double   f64;

typedef enum
{
  CLIB_TYPE_U8      = 0x0003,
  CLIB_TYPE_U16     = 0x0004,
  CLIB_TYPE_U32     = 0x0005,
  CLIB_TYPE_U64     = 0x0006,
  CLIB_TYPE_I8      = 0x0013,
  CLIB_TYPE_I16     = 0x0014,
  CLIB_TYPE_I32     = 0x0015,
  CLIB_TYPE_I64     = 0x0016,
  CLIB_TYPE_F32     = 0x0025,
  CLIB_TYPE_F64     = 0x0026,
  CLIB_TYPE_BOOL    = 0x0030,
  CLIB_TYPE_CHAR    = 0x0040,
  CLIB_TYPE_CSTRING = 0x0041,
  CLIB_TYPE_STRING  = 0x0042
} clibType;

//==============================================================================
#endif //CLIB_TYPES_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

