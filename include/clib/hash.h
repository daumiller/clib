// hash.h
//==============================================================================
#ifndef CLIB_HASH_HEADER
#define CLIB_HASH_HEADER
//==============================================================================
#include <clib/types.h>
//==============================================================================

typedef struct struct_hashKvp
{
  void *key;
  void *val;
  u32   hash;
  struct struct_hashKvp *next;
} hashKvp;

typedef u32  (*hashFunct)     (void *key);
typedef i32  (*hashKeyCompare)(void *keyA, void *keyB);
typedef void (*hashKvpInsert)(hashKvp *kvp, void *key, void *val);
typedef void (*hashKvpAssign)(hashKvp *kvp, void *val);
typedef void (*hashKvpFree)  (hashKvp *kvp);

typedef struct
{
  hashFunct      hash;
  hashKeyCompare keyCompare;
  hashKvpInsert  kvpInsert;
  hashKvpAssign  kvpAssign;
  hashKvpFree    kvpFree;
  f32            loadLo;
  f32            loadHi;
} hashType;

typedef struct
{
  hashType  *type;
  u32        buckets;
  u32        entries;
  u32        mask;
  hashKvp  **bucket;
} hash;

typedef bool (*hashIterator) (hash *table, hashKvp *kvp, void *data);

hash *hashCreate();
hash *hashCreateWithSize(u32 size);
hash *hashCreateWithSizeAndType(u32 size, hashType *type);
void hashFree(hash **table);

bool hashContainsKey(hash *table, void *key);
void hashRemoveKey(hash *table, void *key);
void hashSetValue(hash *table, void *key, void *val);
void *hashGetValue(hash *table, void *key);
void hashIterate(hash *table, hashIterator iter, void *data);

void    **hashKeys(hash *table);
void    **hashVals(hash *table);
hashKvp **hashKvps(hash *table);
void  hashClear(hash *table);
hash *hashDuplicate(hash *table);
void  hashMerge(hash *into, hash *from);

//==============================================================================
#endif //CLIB_HASH_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

