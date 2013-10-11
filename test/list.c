#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <clib/list.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

bool listCounter(list *lst, listItem *item, void *data);

int main(int argc, char **argv)
{
  list *lstA = listCreate();
  char *itemTwo = "List Item 2";
  listAppend(lstA, itemTwo);
  listAppend(lstA, "List Item 3");
  listAppend(lstA, "List Item 4");
  listInsert(lstA, "List Item 0", lstA->origin);
  listInsert(lstA, "List Item 1", lstA->origin->next);
  u32 idxA = 0;
  listIterate(lstA, listCounter, (void *)&idxA);
  printf("Test 0  : %s\n", PASSFAIL(idxA == 5));
  
  listRemove(lstA, itemTwo, true);
  listRemoveItem(lstA, lstA->final->prev);
  listRemoveItem(lstA, lstA->final);
  idxA = 0;
  listIterate(lstA, listCounter, (void *)&idxA);
  printf("Test 1  : %s\n", PASSFAIL(idxA == 2));

  listAppend(lstA, itemTwo);
  listAppend(lstA, "List Item 5"); listItem *tmp = lstA->final;
  listAppend(lstA, "List Item 6");
  listAppend(lstA, "List Item 3");
  listAppend(lstA, "List Item 4");
  listMoveItem(lstA, lstA->final->prev, tmp);
  listMoveItem(lstA, lstA->final, tmp);
  idxA = 0;
  listIterate(lstA, listCounter, (void *)&idxA);
  printf("Test 2  : %s\n", PASSFAIL(idxA == 7));

  printf("Test 3  : %s\n", PASSFAIL(strcmp("List Item 3", (char *)listIndexAt(lstA, 3)) == 0));
  printf("Test 4  : %s\n", PASSFAIL(strcmp("List Item 4", (char *)(listIndexAtItem(lstA,4)->data)) == 0));
  printf("Test 5  : %s\n", PASSFAIL(listIndexOf(lstA, itemTwo, true) == 2));
  printf("Test 6  : %s\n", PASSFAIL(listIndexOfItem(lstA, lstA->final) == 6));
  printf("Test 7  : %s\n", PASSFAIL(listIndexOf(lstA, "(none)", false) == 0xFFFFFFFF));

  list *lstB = listDuplicate(lstA);
  listFree(&lstA);

  idxA = 0;
  listIterate(lstB, listCounter, (void *)&idxA);
  printf("Test 8  : %s\n", PASSFAIL(idxA == 7));

  char **carr = (char **)listToArray(lstB, &idxA);
  listFree(&lstB);
  char buff[32];
  for(u32 i=0; i<idxA; i++)
  {
    sprintf(buff, "List Item %u", i);
    printf("Test |%u : %s\n", i, PASSFAIL(strcmp(buff, carr[i]) == 0));
  }
  printf("Test 9  : %s\n", PASSFAIL(idxA == 7));
  free(carr);

  return 0;
}

bool listCounter(list *lst, listItem *item, void *data)
{
  u32 *idx = (u32 *)data;
  char buff[32];
  sprintf(buff, "List Item %u", *idx);
  printf("Test |%u : %s\n", *idx, PASSFAIL(strcmp(buff, (char *)item->data) == 0));
  (*idx)++;
  return true;
}
