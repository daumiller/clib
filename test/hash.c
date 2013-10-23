#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/hash.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")
bool testFourHelper(hash *table, hashKvp *kvp, void *data);

int main(int argc, char **argv)
{
  hash *table = hashCreate();
  hashSetValue(table, "Basic Test One", "hello world!");
  hashSetValue(table, "Basic Test Two", "hello again");
  printf("Test 0a : %s\n", PASSFAIL(strcmp(hashGetValue(table,"Basic Test One"), "hello world!") == 0));
  printf("Test 0b : %s\n", PASSFAIL(hashGetValue(table,"Basic test Two") == NULL));
  printf("Test 0c : %s\n", PASSFAIL(strcmp(hashGetValue(table,"Basic Test Two"), "hello again") == 0));
  hashSetValue(table, "Basic Test One", "test.basic.3a");
  printf("Test 0d : %s\n", PASSFAIL(strcmp(hashGetValue(table,"Basic Test One"), "test.basic.3a") == 0));

  printf("Test 1a : %s\n", PASSFAIL(hashContainsKey(table, "Basic Test Two") == true ));
  printf("Test 1b : %s\n", PASSFAIL(hashContainsKey(table, "Basic Test TwO") == false));

  hashSetValue(table, "keyThree", (void *)1024);
  printf("Test 2a : %s\n", PASSFAIL(hashContainsKey(table,"keyThree") == true));
  printf("Test 2b : %s\n", PASSFAIL((int)hashGetValue(table,"keyThree") == 1024));
  hashRemoveKey(table, "keyThree");
  printf("Test 2c : %s\n", PASSFAIL(hashContainsKey(table,"keyThree") == false));
  printf("Test 2d : %s\n", PASSFAIL(hashGetValue(table,"keyThree") == NULL));

  printf("Test 3a : %s\n", PASSFAIL(hashContainsKey(table,"Basic Test One") == true ));
  printf("Test 3b : %s\n", PASSFAIL(hashContainsKey(table,"Basic Test Two") == true ));
  printf("Test 3c : %s\n", PASSFAIL(hashContainsKey(table,"keyThree"      ) == false));
  hashClear(table);
  printf("Test 3d : %s\n", PASSFAIL(hashContainsKey(table,"Basic Test One") == false));
  printf("Test 3e : %s\n", PASSFAIL(hashContainsKey(table,"Basic Test Two") == false));
  printf("Test 3f : %s\n", PASSFAIL(hashContainsKey(table,"keyThree"      ) == false));
  hashFree(&table);

  table = hashCreateWithSize(2048);
  char buff[7];
  for(u32 i=0; i<32; i++)
  {
    sprintf(buff, "test%d", (int)i);
    hashSetValue(table, buff, (void *)(intptr_t)i);
  }
  u32 countIndex = 0;
  hashIterate(table, testFourHelper, (void *)&countIndex);
  printf("Test 4a : %s\n", PASSFAIL(countIndex == 32));
  for(u32 i=8; i<12; i++)
  {
    sprintf(buff, "test%d", (int)i);
    hashRemoveKey(table, buff);
  }
  countIndex = 0;
  hashIterate(table, testFourHelper, (void *)&countIndex);
  printf("Test 4b : %s\n", PASSFAIL(countIndex == 28));
  hashClear(table);
  countIndex = 0;
  hashIterate(table, testFourHelper, (void *)&countIndex);
  printf("Test 4c : %s\n", PASSFAIL(countIndex == 0));

  hashSetValue(table, "key1" , "val0");
  hashSetValue(table, "key2" , "val1");
  hashSetValue(table, "key4" , "val?");
  hashSetValue(table, "key8" , "val3");
  hashSetValue(table, "key16", "val4");
  hash *tableB = hashDuplicate(table);
  char **keysA = (char **)hashKeys(table);  char **valsA = (char **)hashVals(table);
  char **keysB = (char **)hashKeys(tableB); char **valsB = (char **)hashVals(tableB);
  printf("Test 5a : %s\n", PASSFAIL((table->entries == tableB->entries) && (table->buckets == tableB->buckets)));
  bool fiveB = true, fiveC = true;
  for(u32 i=0; i<table->entries; i++)
  {
    if(keysA[i] == keysB[i])           fiveB = false;
    if(strcmp(keysA[i],keysB[i]) != 0) fiveB = false;
    if(valsA[i] != valsB[i])           fiveC = false;
    if(strcmp(valsA[i],valsB[i]) != 0) fiveC = false;
  }
  printf("Test 5b : %s\n", PASSFAIL(fiveB));
  printf("Test 5c : %s\n", PASSFAIL(fiveC));
  free(keysA); free(valsA);
  free(keysB); free(valsB);
  hashFree(&tableB);

  tableB = hashCreateWithSizeAndType(table->buckets, table->type);
  hashSetValue(tableB, "key4" , "val2");
  hashSetValue(tableB, "key32", "val5");
  hashSetValue(tableB, "key64", "val6");
  hashMerge(table, tableB);
  hashFree(&tableB);
  printf("Test 6a : %s\n", PASSFAIL(table->entries == 7));
  char keybuff[8], valbuff[8];
  char sub[] = { 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
  for(u32 i=0; i<7; i++)
  {
    sprintf(valbuff, "val%d", i);
    sprintf(keybuff, "key%d", 1<<i);
    printf("Test 6%c : %s\n", sub[i], PASSFAIL(strcmp((char *)hashGetValue(table,keybuff),valbuff) == 0) );
  }

  hashFree(&table);
  return 0;
}

bool testFourHelper(hash *table, hashKvp *kvp, void *data)
{
  u32 *index = (u32 *)data;
  (*index)++;
  return true;
}
