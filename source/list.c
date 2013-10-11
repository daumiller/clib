// list.c
//==============================================================================
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/list.h>
//==============================================================================
static listItem *listItemForData(list *lst, void *data, bool exact);
//==============================================================================
static void *listDefault_insert(void *data);
static void  listDefault_free(void *data);
static i32   listDefault_compare(void *dataA, void *dataB);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static listType listTypeDefault = {
  .dataInsert  = (listDataInsert )listDefault_insert,
  .dataCopy    = (listDataCopy   )listDefault_insert,
  .dataFree    = (listDataFree   )listDefault_free  ,
  .dataCompare = (listDataCompare)listDefault_compare
};
//==============================================================================

list *listCreate() { return listCreateWithType(&listTypeDefault); }

list *listCreateWithType(listType *type)
{
  list *lst = (list *)malloc(sizeof(list));
  lst->type   = type;
  lst->origin = NULL;
  lst->final  = NULL;
  lst->count  = 0;
  return lst;
}

void listFree(list **lst)
{
  listItem *tmp, *curr = (*lst)->origin;
  while(curr)
  {
    (*lst)->type->dataFree(curr->data);
    tmp  = curr;
    curr = curr->next;
    free(tmp);
  }
  free(*lst);
  *lst = NULL;
}

//------------------------------------------------------------------------------

void listAppend(list *lst, void *data)
{
  listItem *item = (listItem *)malloc(sizeof(listItem));
  item->data = lst->type->dataInsert(data);
  listAppendItem(lst, item);
}

void listAppendItem(list *lst, listItem *item)
{
  item->prev = lst->final;
  item->next = NULL;
  if(lst->final) lst->final->next = item;
  lst->final = item;
  if(lst->origin == NULL) lst->origin = item;
  lst->count++;
}

void listInsert(list *lst, void *data, listItem *before)
{
  listItem *item = (listItem *)malloc(sizeof(listItem));
  item->data = lst->type->dataInsert(data);
  listInsertItem(lst, item, before);
}

void listInsertItem(list *lst, listItem *item, listItem *before)
{
  if(before == NULL)
  {
    item->prev = lst->final;
    item->next = NULL;
    if(lst->final) lst->final->next = item;
    lst->final = item;
    if(lst->origin == NULL) lst->origin = item;
  }
  else
  {
    item->prev   = before->prev;
    item->next   = before;
    before->prev = item;
    if(item->prev) item->prev->next = item;
    if(lst->origin == before) lst->origin = item;
  }
  lst->count++;
}

//------------------------------------------------------------------------------

void listRemove(list *lst, void *data, bool exact)
{
  listItem *item = listItemForData(lst, data, exact);
  if(item) listRemoveItem(lst, item);
}

void listRemoveItem(list *lst, listItem *item)
{
  if(lst->origin == item) lst->origin = item->next;
  if(lst->final  == item) lst->final  = item->prev;
  if(item->prev) item->prev->next = item->next;
  if(item->next) item->next->prev = item->prev;
  lst->type->dataFree(item->data);
  free(item);
  lst->count--;
}

//------------------------------------------------------------------------------

void listMoveItem(list *lst, listItem *item, listItem *before)
{
  if(lst->origin == item) lst->origin = item->next;
  if(lst->final  == item) lst->final  = item->prev;
  if(item->prev) item->prev->next = item->next;
  if(item->next) item->next->prev = item->prev;

  if(before == NULL)
  {
    if(lst->final) lst->final->next = item;
    item->prev = lst->final;
    item->next = NULL;
    lst->final = item;
  }
  else
  {
    if(lst->origin == before) lst->origin = item;
    item->prev   = before->prev;
    item->next   = before;
    before->prev = item;
    if(item->prev) item->prev->next = item;
  }
}

void *listIndexAt(list *lst, u32 index)
{
  listItem *curr = listIndexAtItem(lst, index);
  if(curr) return curr->data;
  return NULL;
}

listItem *listIndexAtItem(list *lst, u32 index)
{
  listItem *curr = lst->origin;
  u32 i = 0;
  while(curr && (i < index))
  {
    curr = curr->next;
    i++;
  }
  return curr;
}

u32 listIndexOf(list *lst, void *data, bool exact)
{
  listItem *curr = lst->origin;
  u32 i = 0;
  while(curr)
  {
    if(exact)
    {
      if(curr->data == data)
        return i;
    }
    else
    {
      if(lst->type->dataCompare(curr->data, data) == 0)
        return i;
    }
    curr = curr->next;
    i++;
  }
  return 0xFFFFFFFF;
}

u32 listIndexOfItem(list *lst, listItem *item)
{
  listItem *curr = lst->origin;
  u32 i = 0;
  while(curr)
  {
    if(curr == item) return i;
    curr = curr->next;
    i++;
  }
  return 0xFFFFFFFF;
}

//------------------------------------------------------------------------------

void listIterate(list *lst, listIterator iterator, void *data)
{
  listItem *curr = lst->origin;
  bool iter = true;
  while(curr && iter)
  {
    iter = iterator(lst, curr, data);
    curr = curr->next;
  }
}

list *listDuplicate(list *lst)
{
  list *ret = (list *)malloc(sizeof(list));
  ret->type  = lst->type;
  ret->count = lst->count;

  listItem *cpy, *cpyPrv=NULL, *from = lst->origin;
  while(from)
  {
    cpy = (listItem *)malloc(sizeof(listItem));
    cpy->data = lst->type->dataCopy(from->data);
    cpy->prev = cpyPrv;
    cpy->next = NULL;
    if(cpyPrv) cpyPrv->next = cpy;
    if(from == lst->origin) ret->origin = cpy;
    if(from == lst->final ) ret->final  = cpy;
    from = from->next;
    cpyPrv = cpy;
  }

  return ret;
}

void **listToArray(list *lst, u32 *count)
{
  void **data = (void **)malloc(sizeof(void *) * lst->count);
  listItem *curr = lst->origin;
  u32 i=0;
  while(curr)
  {
    data[i] = lst->type->dataCopy(curr->data);
    curr = curr->next;
    i++;
  }
  if(count) *count = lst->count;
  return data;
}

//==============================================================================

static void *listDefault_insert(void *data)
{
  return data;
}

static void listDefault_free(void *data)
{
}

static i32 listDefault_compare(void *dataA, void *dataB)
{
  return (dataA == dataB) ? 0 : 1;
}

//==============================================================================

static listItem *listItemForData(list *lst, void *data, bool exact)
{
  listItem *curr = lst->origin;
  while(curr)
  {
    if(exact)
    {
      if(curr->data == data)
        return curr;
    }
    else
    {
      if(lst->type->dataCompare(curr->data, data) == 0)
        return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

