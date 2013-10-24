// fileSystem.c
//==============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __linux__
# include <malloc.h>
//readdir_r & DT_DIR
# define __USE_MISC
# define __USE_BSD
# include <dirent.h>
//nftw
# define __USE_XOPEN_EXTENDED
# include <ftw.h>
//Force thread-safe functions
# define __USE_MISC
# include <pwd.h>
//Force non-gnu strerror_r
# undef __USE_MISC
# include <string.h>
  extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);
# define strerror_r __xpg_strerror_r
# define S_IFMT  __S_IFMT
# define S_IFREG __S_IFREG
# define S_IFBLK __S_IFBLK
# define S_IFCHR __S_IFCHR
# define S_IFDIR __S_IFDIR
#else
# include <dirent.h>
# include <pwd.h>
# include <string.h>
# include <ftw.h>
#endif
#include <clib/fileSystem.h>
#include <clib/string.h>
//==============================================================================
#define REASONABLY_LARGE_BUFFER 8192
//==============================================================================

bool fsPathExists(char *path)
{
  struct stat st;
  return (stat(path, &st) == 0);
}

bool fsPathIsFile(char *path)
{
  struct stat st;
  if(stat(path, &st) != 0) return false;
  if((st.st_mode & S_IFMT) & S_IFDIR) return false;
  return (((st.st_mode & S_IFMT) & (S_IFREG | S_IFBLK | S_IFCHR)) != 0);
}

bool fsPathIsDirectory(char *path)
{
  struct stat st;
  if(stat(path, &st) != 0) return false;
  return (((st.st_mode & S_IFMT) & S_IFDIR) != 0);
}

char *fsPathNormalize(char *path)
{
  //split by '/'
  string *clstr = stringFromCString(path);
  u32 tokens = 0;
  string **nodes = stringSplit(clstr, "/", 1, &tokens);
  free(clstr);

  if(tokens == 0)
  {
    for(u32 i=0; i<tokens; i++) free(nodes[i]); free(nodes);
    if(path[0] == 0x00) return fsPathCurrent();
    if(path[0] == '/' ) return clstrdup("/");
    return NULL;
  }

  //find leader ('/', '.', '~')
  stringBuilder *sb = stringBuilderCreate();
  u32 tokIdx = 0;
  if(path[0] != '/')
  {
    if((nodes[0]->length == 1) && (nodes[0]->data[0] == '~'))
    {
      tokIdx = 1;
      char *home = fsPathHome();
      stringBuilderAppendCString(sb, home);
      free(home);
    }
    else
    {
      char *cwd = fsPathCurrent();
      stringBuilderAppendCString(sb, cwd);
      free(cwd);
    }
  }

  //append nodes
  for(; tokIdx < tokens; tokIdx++)
  {
    if(nodes[tokIdx]->length == 0) continue;
    if(nodes[tokIdx]->length == 1) if(nodes[tokIdx]->data[0] == '.') continue;
    if(nodes[tokIdx]->length == 2) if(nodes[tokIdx]->data[0] == '.') if(nodes[tokIdx]->data[1] == '.')
      { stringBuilderPop(sb); stringBuilderPop(sb); continue; }
    stringBuilderAppendCharacter(sb, '/', 1);
    stringBuilderAppendString(sb, nodes[tokIdx]);
  }
  if(sb->length == 0) stringBuilderAppendCharacter(sb, '/', 1);

  //combine & cleanup
  for(u32 i=0; i<tokens; i++) free(nodes[i]); free(nodes);
  char *normal = stringBuilderToCString(sb);
  stringBuilderFree(&sb);
  return normal;
}

char *fsPathBase(char *path)
{
  if(path[0] == 0x00) return NULL;
  char *slash = path;
  char *term  = path;
  while(*term != 0x00)
  {
    if(*term == '/') slash=term;
    term++;
  }
  slash++;
  return clstrdupn(slash, term-slash);
}

char *fsPathParent(char *path)
{
  if(path[0] == 0x00) return NULL;
  char *slash = path;
  char *term  = path;
  while(*term != 0x00)
  {
    if(*term == '/') slash=term;
    term++;
  }
  return clstrdupn(path, slash-path);
}

char *fsPathCurrent()
{
  char *buff = (char *)malloc(REASONABLY_LARGE_BUFFER);
  char *result = getcwd(buff, REASONABLY_LARGE_BUFFER);
  if(!result) { free(buff); return NULL; }
  result = clstrdup(result);
  free(buff);
  return result;
}

char *fsPathHome()
{
  char *home = getenv("HOME");
  if(home) return clstrdup(home);

  char *buff = (char *)malloc(REASONABLY_LARGE_BUFFER);
  struct passwd pwd, *result;
  getpwuid_r(getuid(), &pwd, buff, REASONABLY_LARGE_BUFFER, &result);
  if(result == NULL) { free(buff); return NULL; }

  char *ret = clstrdup(pwd.pw_dir);
  free(buff);
  return ret;
}

//==============================================================================

bool fsDirectoryCreate(char *path, i32 *status, char **message)
{
  //check if we already exist
  if(fsPathIsDirectory(path))
  {
    if(status ) *status = 0;
    if(message) *message = NULL;
    return true;
  }

  //attempt to recursively create needed parents
  char *parent = fsPathParent(path);
  if(parent)
  {
    if(parent[0] != 0x00)
      if(fsDirectoryCreate(parent, status, message) == false)
        { free(parent); return false; }
    free(parent);
  }

  //create base directory
  int result = mkdir(path, 0777);
  if(result == 0)
  {
    if(status ) *status  = 0;
    if(message) *message = NULL;
    return true;
  }
  else
  {
    if(status ) *status = errno;
    if(message)
    {
      char buff[1024];
      *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
    }
    return false;
  }
}

static int fsDirectoryDelete_helper(const char *path, const struct stat *st, int flag, struct FTW *ftw)
  { return remove(path); }
bool fsDirectoryDelete(char *path, i32 *status, char **message)
{
  //this is always a recursive delete
  int result = nftw(path, fsDirectoryDelete_helper, 32, FTW_DEPTH | FTW_PHYS);
  if(result == 0)
  {
    if(status ) *status  = 0;
    if(message) *message = NULL;
    return true;
  }

  if(status) *status = errno;
  if(message)
  {
    char buff[1024];
    *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
  }
  return false;
}

bool fsDirectoryMove(char *pathOld, char *pathNew, i32 *status, char **message)
{
  int result = rename(pathOld, pathNew);
  if(result == 0)
  {
    if(status ) *status  = 0;
    if(message) *message = NULL;
    return true;
  }

  if(status) *status = errno;
  if(message)
  {
    char buff[1024];
    *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
  }
  return false;
}

static void *fsDirList_InsCpy(void *data) { return data; }
static i32   fsDirList_Cmp   (void *a, void *b) { return (a == b) ? 0 : 1; }
static void fsDirList_Free(fsDirectoryItem *item)
{
  if(item->name) free(item->name);
  free(item);
}
static listType fsDirList_ListType = {
  .dataInsert  = (listDataInsert )fsDirList_InsCpy,
  .dataCopy    = (listDataCopy   )fsDirList_InsCpy,
  .dataCompare = (listDataCompare)fsDirList_Cmp   ,
  .dataFree    = (listDataFree   )fsDirList_Free
};
void fsDirectoryList_Helper(char *path, DIR *dir, list *lst, regex *filter, int flags)
{
  struct dirent *entry, *result;
  u32 len = offsetof(struct dirent, d_name) + pathconf(path, _PC_NAME_MAX) + 1;
  entry = (struct dirent *)malloc(len);

  int status = 0; result = entry;
  while((status == 0) && (result != NULL))
  {
    status = readdir_r(dir, entry, &result);
    if(status != 0   ) break;
    if(result == NULL) break;

    bool isDir = (entry->d_type == DT_DIR);
    if(isDir) { if(flags & FS_DIRLIST_NODIRS ) continue; } else { if(flags & FS_DIRLIST_NOFILES) continue; }
    if(entry->d_name[0] == '.')
    {
      if(flags & FS_DIRLIST_NOHIDDEN) continue;
      if(entry->d_name[1] == 0x00) if((flags & FS_DIRLIST_DOTDOT) == 0) continue;
      if(entry->d_name[1] == '.' ) if(entry->d_name[2] == 0x00) if((flags & FS_DIRLIST_DOTDOT) == 0) continue;
    }
    if(filter) if(regexIsMatch(filter, entry->d_name) == false) continue;

    fsDirectoryItem *fsdi = (fsDirectoryItem *)malloc(sizeof(fsDirectoryItem));
    fsdi->name = clstrdup(entry->d_name);
    fsdi->isDirectory = isDir;
    listAppend(lst, fsdi);
  }
  free(entry);
}
list *fsDirectoryList(char *path, int flags, regex *filter)
{
  DIR *dir = opendir(path);
  if(dir == NULL) return NULL;

  list *lst = listCreateWithType(&fsDirList_ListType);

  if(flags & FS_DIRLIST_DIRSFIRST)
  {
    fsDirectoryList_Helper(path, dir, lst, filter, flags | FS_DIRLIST_NOFILES);
    rewinddir(dir);
  }
  if(flags & FS_DIRLIST_DIRSFIRST) flags |= FS_DIRLIST_NODIRS;
  fsDirectoryList_Helper(path, dir, lst, filter, flags);

  closedir(dir);
  return lst;
}

//==============================================================================

bool fsFileCreate(char *path, i32 *status, char **message)
{
  FILE *fc = fopen(path, "wb");
  if(fc)
  {
    fclose(fc);
    if(status ) *status  = 0;
    if(message) *message = NULL;
    return true;
  }

  if(status ) *status = errno;
  if(message)
  {
    char buff[1024];
    *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
  }
  return false;
}

bool fsFileDelete(char *path, i32 *status, char **message)
{
  int result = remove(path);
  if(result == 0)
  {
    if(status ) *status  = 0;
    if(message) *message = NULL;
    return true;
  }

  if(status ) *status = errno;
  if(message)
  {
    char buff[1024];
    *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
  }
  return false;
}

bool fsFileMove(char *pathOld, char *pathNew, i32 *status, char **message)
{
  return fsDirectoryMove(pathOld, pathNew, status, message);
}

u64 fsFileSize(char *path)
{
  struct stat st;
  if(stat(path, &st) != 0) return 0;
  return (u64)st.st_size;
}

char *fsFileRead(char *path, i32 *status, char **message)
{
  if(!fsPathIsFile(path))
  {
    if(status ) *status  = EACCES;
    if(message) *message = clstrdup("Non-existant file");
    return NULL;
  }

  FILE *fin = fopen(path, "rb");
  if(!fin)
  {
    if(status ) *status = errno;
    if(message)
    {
      char buff[1024];
      *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
    }
    return NULL;
  }

  u64 size = fsFileSize(path);
  char *buff = (char *)malloc(size);
  if(!buff)
  {
    fclose(fin);
    if(status ) *status  = ENOMEM;
    if(message) *message = clstrdup("Unable to allocate enough memory");
    return NULL;
  }

  u64 read = 0;
  while(read < size)
    read += fread(buff+read, 1, size-read, fin);

  fclose(fin);
  return buff;
}

bool fsFileWrite(char *path, void *content, u32 size, i32 *status, char **message)
{
  FILE *fout = fopen(path, "wb");
  if(!fout)
  {
    if(status ) *status = errno;
    if(message)
    {
      char buff[1024];
      *message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown error");
    }
    return false;
  }

  u32 wrote = 0;
  while(wrote < size)
    wrote += fwrite(content+wrote, 1, size-wrote, fout);

  fclose(fout);
  return true;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
