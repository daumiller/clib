// hash.c
//==============================================================================
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <clib/types.h>
#include <clib/string.h>
#include <clib/hash.h>
//==============================================================================
static void hashLookup(hash *table, void *key, u32 *hash, u32 *index, hashKvp **sibling, hashKvp **kvp);
static void hashShrink(hash *table);
static void hashGrow(hash *table);
static void hashResize(hash *table, u32 size);
//==============================================================================
static u32  hashDefault_Hash(char *key);
static i32  hashDefault_KeyCompare(char *keyA, char *keyB);
static void hashDefault_KvpInsert(hashKvp *kvp, char *key, void *val);
static void hashDefault_KvpAssign(hashKvp *kvp, void *val);
static void hashDefault_KvpFree  (hashKvp *kvp);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static hashType hashTypeDefault = {
  .hash       = (hashFunct)     hashDefault_Hash,
  .keyCompare = (hashKeyCompare)hashDefault_KeyCompare,
  .kvpInsert  = (hashKvpInsert) hashDefault_KvpInsert,
  .kvpAssign  = (hashKvpAssign) hashDefault_KvpAssign,
  .kvpFree    = hashDefault_KvpFree,
  .loadLo     = 0.0078125f, // 1/128 -> Shrink
  .loadHi     = 0.75f,      // 3/4   -> Grow
};
//==============================================================================

hash *hashCreate()                { return hashCreateWithSizeAndType( 128, &hashTypeDefault); }
hash *hashCreateWithSize(u32 size){ return hashCreateWithSizeAndType(size, &hashTypeDefault); }
hash *hashCreateWithSizeAndType(u32 size, hashType *type)
{
  u32 sizeP2 = 1;
  while(sizeP2 < size) sizeP2 <<= 1;

  hash *h = (hash *)malloc(sizeof(hash));
  h->type    = type;
  h->buckets = sizeP2;
  h->entries = 0;
  h->mask    = sizeP2-1;
  h->bucket  = (hashKvp **)calloc(1, sizeof(hashKvp *) * sizeP2);
  return h;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashFree(hash **table)
{
  hashClear(*table);
  free((*table)->bucket);
  free(*table);
  *table = NULL;
}

//==============================================================================

bool hashContainsKey(hash *table, void *key)
{
  hashKvp *kvp;
  hashLookup(table, key, NULL, NULL, NULL, &kvp);
  return (kvp != NULL);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashRemoveKey(hash *table, void *key)
{
  hashKvp *sibling, *kvp; u32 index;
  hashLookup(table, key, NULL, &index, &sibling, &kvp);
  if(kvp == NULL) return; //not found

  table->type->kvpFree(kvp);
  
  if(sibling != kvp)
  {
    sibling->next = kvp->next;
  }
  else if(kvp->next != NULL)
  {
    kvp->key  = kvp->next->key;
    kvp->val  = kvp->next->val;
    kvp->hash = kvp->next->hash;
    kvp->next = kvp->next->next;
    if(table->bucket[index] == kvp)
      table->bucket[index] = kvp->next;
    kvp = kvp->next;
  }
  else
  {
    if(table->bucket[index] == kvp)
      table->bucket[index] = NULL;
  }
  free(kvp);
  table->entries--;
  float load = ((float)table->entries) / ((float)table->buckets);
  if(load < table->type->loadLo) hashShrink(table);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashSetValue(hash *table, void *key, void *val)
{
  u32 hash, index;
  hashKvp *sibling, *kvp;
  hashLookup(table, key, &hash, &index, &sibling, &kvp);

  //re-assigning value
  if(kvp != NULL)
  {
    table->type->kvpAssign(kvp, val);
    return;
  }

  //inserting a value
  kvp = (hashKvp *)malloc(sizeof(hashKvp));
  table->type->kvpInsert(kvp, key, val);
  kvp->hash = hash;
  kvp->next = NULL;

  //bucket or chain?
  if(sibling != NULL)
    sibling->next = kvp;
  else
    table->bucket[index] = kvp;

  table->entries++;
  float load = ((float)table->entries) / ((float)table->buckets);
  if(load > table->type->loadHi) hashGrow(table);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void *hashGetValue(hash *table, void *key)
{
  hashKvp *kvp;
  hashLookup(table, key, NULL, NULL, NULL, &kvp);
  if(kvp == NULL) return NULL;
  return kvp->val;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashIterate(hash *table, hashIterator iter, void *data)
{
  hashKvp *entry;
  for(u32 i=0; i<table->buckets; i++)
  {
    entry = table->bucket[i];
    while(entry)
    {
      if(!iter(table, entry, data)) return;
      entry = entry->next;
    }
  }
}

//==============================================================================

void **hashKeys(hash *table)
{
  void **keys = (void **)malloc(sizeof(void *) * table->entries);
  hashKvp *curr;
  for(u32 i=0, idx=0; i<table->buckets; i++)
  {
    curr = table->bucket[i];
    while(curr != NULL)
    {
      keys[idx] = curr->key;
      curr = curr->next;
      idx++;
    }
  }
  return keys;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void **hashVals(hash *table)
{
  void **vals = (void **)malloc(sizeof(void *) * table->entries);
  hashKvp *curr;
  for(u32 i=0, idx=0; i<table->buckets; i++)
  {
    curr = table->bucket[i];
    while(curr != NULL)
    {
      vals[idx] = curr->val;
      curr = curr->next;
      idx++;
    }
  }
  return vals;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
hashKvp **hashKvps(hash *table)
{
  hashKvp **kvps = (hashKvp **)malloc(sizeof(hashKvp *) * table->entries);
  hashKvp *curr;
  for(u32 i=0, idx=0; i<table->buckets; i++)
  {
    curr = table->bucket[i];
    while(curr != NULL)
    {
      kvps[idx] = curr;
      curr = curr->next;
      idx++;
    }
  }
  return kvps;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashClear(hash *table)
{
  hashKvp *curr, *next;

  for(u32 i=0; i<table->buckets; i++)
  {
    curr = table->bucket[i];
    while(curr != NULL)
    {
      table->type->kvpFree(curr);
      next = curr->next;
      free(curr);
      curr = next;
    }
    table->bucket[i] = NULL;
  }
  table->entries = 0;
  //not shrinking from here...
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
hash *hashDuplicate(hash *table)
{
  hash *dup = hashCreateWithSizeAndType(table->buckets, table->type);
  hashKvp *from, *to, *ins;
  for(u32 i=0; i<table->buckets; i++)
  {
    from = table->bucket[i];
    to   = dup->bucket[i];
    while(from)
    {
      ins = (hashKvp *)calloc(1,sizeof(hashKvp));
      table->type->kvpInsert(ins, from->key, from->val);
      if(to == NULL) dup->bucket[i] = ins; else to->next=ins;
      to = ins;
      from = from->next;
    }
  }
  dup->entries = table->entries;
  return dup;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void hashMerge(hash *into, hash *from)
{
  hashKvp *curr;
  for(u32 i=0; i<from->buckets; i++)
  {
    curr = from->bucket[i];
    while(curr)
    {
      hashSetValue(into, curr->key, curr->val);
      curr = curr->next;
    }
  }
}

//==============================================================================

static void hashLookup(hash *table, void *key, u32 *hash, u32 *index, hashKvp **sibling, hashKvp **kvp)
{
  u32 h, i; hashKvp *s, *k;
  if(hash    == NULL) hash    = &h; *hash    = table->type->hash(key);
  if(index   == NULL) index   = &i; *index   = *hash & table->mask;
  if(sibling == NULL) sibling = &s; *sibling = table->bucket[*index];
  if(kvp     == NULL) kvp     = &k; *kvp     = NULL;

  if(*sibling == NULL) return; //bucket not populated

  *kvp = *sibling;
  while(*kvp)
  {
    if((*kvp)->hash == *hash)
      if(table->type->keyCompare((*kvp)->key, key) == 0)
        return; //found exact match
    *sibling = *kvp;
    *kvp = (*sibling)->next;
  }
  //bucket populated, but no exact match
}

//------------------------------------------------------------------------------

static void hashShrink(hash *table){ hashResize(table, table->buckets >> 1); }
static void hashGrow(hash *table)  { hashResize(table, table->buckets << 1); }
static void hashResize(hash *table, u32 size)
{
  u32 oldBuckets = table->buckets;
  hashKvp **oldBucket = table->bucket;

  table->buckets = size;
  table->mask    = size-1;
  table->bucket  = (hashKvp **)calloc(1, sizeof(hashKvp *) * size);

  hashKvp *curr;
  for(u32 i=0,index; i<oldBuckets; i++)
  {
    oldBucket[i]->next = NULL;
    index = oldBucket[i]->hash & table->mask;
    if(table->bucket[index] == NULL)
      table->bucket[index] = oldBucket[i];
    else
    {
      curr = table->bucket[index];
      while(curr->next != NULL) curr = curr->next;
      curr->next = oldBucket[i];
    }
  }
  free(oldBucket);
}

//==============================================================================

static u32 hashDefault_Hash(char *key)
{
  //"Jenkins One at a Time" hash:
  u32 hash = 0; // (0|seed)
  while(*key)
  {
    hash += *key;
    hash += (hash << 10);
    hash ^= (hash >>  6);
    key++;
  }
  hash += (hash <<  3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static i32 hashDefault_KeyCompare(char *keyA, char *keyB)
{
  return strcmp(keyA, keyB);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void hashDefault_KvpInsert(hashKvp *kvp, char *key, void *val)
{
  kvp->key = strdup(key);
  kvp->val = val;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void hashDefault_KvpAssign(hashKvp *kvp, void *val)
{
  kvp->val = val;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void hashDefault_KvpFree(hashKvp *kvp)
{
  free(kvp->key);
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

