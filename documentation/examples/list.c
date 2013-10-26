#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clib/string.h>
#include <clib/list.h>

// list iterator printing function
bool iterator(list *lst, listItem *item, void *data);

// a custom *listType* to handle c strings
void *stringList_Insert(void *data);
void  stringList_Free(void *data);
i32   stringList_Compare(void *dataA, void *dataB);
listType stringList =
{
  .dataInsert  = stringList_Insert,
  .dataCopy    = stringList_Insert,
  .dataFree    = stringList_Free  ,
  .dataCompare = stringList_Compare
};

int main(int argc, char **argv)
{
  // create a list, add some items
  list *lst = listCreateWithType(&stringList);
  listAppend(lst, "Item One"  );
  listAppend(lst, "Item Two"  );
  listAppend(lst, "Item Three");

  listAppend(lst, "Item Five" );

  // insert a 'zero' item before our 'one' item
  listInsert(lst, "Item Zero", listItemWithData(lst, "Item One", false));

  // remove the 'five' item (will use *listType*->dataCompare)
  listRemove(lst, "Item Five", false);

  // iterate the list with our printing function
  listIterate(lst, iterator, NULL);

  // cleanup
  listFree(&lst);
  return 0;
}
 
bool iterator(list *lst, listItem *item, void *data)
{
  printf("%s\n", (char *)(item->data));
  return true;
}

void *stringList_Insert(void *data)
{
  //create a copy of the input data (cstr) when inserting
  return clstrdup((char *)data);
}
void stringList_Free(void *data)
{
  //free our clstrdup()ed string when cleaning up
  free(data);
}
i32 stringList_Compare(void *dataA, void *dataB)
{
  //use strcmp() as our comparison function (for non-exact matching)
  return strcmp((char *)dataA, (char *)dataB);
}
