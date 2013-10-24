// fileSystem.h
//==============================================================================
#ifndef CLIB_FILESYSTEM_HEADER
#define CLIB_FILESYSTEM_HEADER
//==============================================================================
#include <clib/types.h>
#include <clib/list.h>
#include <clib/regex.h>
//==============================================================================
typedef enum
{
  FS_DIRLIST_DEFAULT   =  0,
  FS_DIRLIST_NOHIDDEN  =  1,
  FS_DIRLIST_DOTDOT    =  2,
  FS_DIRLIST_NODIRS    =  4,
  FS_DIRLIST_NOFILES   =  8,
  FS_DIRLIST_DIRSFIRST = 16
} FS_DIRLIST_FLAGS;
//==============================================================================

bool fsPathExists     (char *path);
bool fsPathIsFile     (char *path);
bool fsPathIsDirectory(char *path);

char *fsPathNormalize(char *path);
char *fsPathBase     (char *path);
char *fsPathParent   (char *path);

char *fsPathCurrent();
char *fsPathHome();

//------------------------------------------------------------------------------

typedef struct
{
  char *name;
  bool  isDirectory;
} fsDirectoryItem;

bool  fsDirectoryCreate(char *path, i32 *status, char **message);
bool  fsDirectoryDelete(char *path, i32 *status, char **message);
bool  fsDirectoryMove  (char *pathOld, char *pathNew, i32 *status, char **message);
list *fsDirectoryList  (char *path, int flags, regex *filter);

//------------------------------------------------------------------------------

bool  fsFileCreate(char *path, i32 *status, char **message);
bool  fsFileDelete(char *path, i32 *status, char **message);
bool  fsFileMove  (char *pathOld, char *pathNew, i32 *status, char **message);
u64   fsFileSize  (char *path);
char *fsFileRead  (char *path, i32 *status, char **message);
bool  fsFileWrite (char *path, void *content, u32 size, i32 *status, char **message);

//==============================================================================
#endif //CLIB_FILESYSTEM_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
