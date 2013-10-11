// list.h
//==============================================================================
#ifndef CLIB_LIST_HEADER
#define CLIB_LIST_HEADER
//==============================================================================
#include <clib/types.h>
//==============================================================================

typedef void *(*listDataInsert )(void *data);
typedef void *(*listDataCopy   )(void *data);
typedef void  (*listDataFree   )(void *data);
typedef i32   (*listDataCompare)(void *dataA, void *dataB);

typedef struct
{
  listDataInsert  dataInsert;
  listDataCopy    dataCopy;
  listDataFree    dataFree;
  listDataCompare dataCompare;
} listType;

typedef struct clibListItem
{
  void *data;
  struct clibListItem *prev;
  struct clibListItem *next;
} listItem;

typedef struct
{
  listType *type;
  listItem *origin;
  listItem *final;
  u32       count;
} list;

typedef bool (*listIterator)(list *lst, listItem *item, void *data);

list *listCreate();
list *listCreateWithType(listType *type);
void listFree(list **lst);
void listAppend(list *lst, void *data);
void listAppendItem(list *lst, listItem *item);
void listInsert(list *lst, void *data, listItem *before);
void listInsertItem(list *lst, listItem *item, listItem *before);
void listRemove(list *lst, void *data, bool exact);
void listRemoveItem(list *lst, listItem *item);
void listMoveItem(list *lst, listItem *item, listItem *before);
void *listIndexAt(list *lst, u32 index);
listItem *listIndexAtItem(list *lst, u32 index);
u32 listIndexOf(list *lst, void *data, bool exact);
u32 listIndexOfItem(list *lst, listItem *item);
void listIterate(list *lst, listIterator iterator, void *data);
list *listDuplicate(list *lst);
void **listToArray(list *lst, u32 *count);

//==============================================================================
#endif //CLIB_LIST_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

