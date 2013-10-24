#include <stdio.h>
#include <string.h>
#ifdef __linux__
# include <malloc.h>
#elif __APPLE__
# include <stdlib.h>
#endif
#include <clib.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

int main(int argc, char **argv)
{
  printf("fsPathExists       0 : %s\n", PASSFAIL(fsPathExists("/") == true));
  printf("fsPathExists       1 : %s\n", PASSFAIL(fsPathExists("/fakePath") == false));

  char *pathTestFile = "fileSystem.test.file";
  char *pathTestDir  = "fileSystem.test.dir" ;

  fsFileDelete(pathTestFile, NULL, NULL); //don't care...
  printf("fsPathExists       2 : %s\n", PASSFAIL(fsPathExists(pathTestFile) == false));
  printf("fsPathIsFile       3 : %s\n", PASSFAIL(fsPathIsFile(pathTestFile) == false));
  printf("fsFileCreate       4 : %s\n", PASSFAIL(fsFileCreate(pathTestFile, NULL, NULL) == true));
  printf("fsPathExists       5 : %s\n", PASSFAIL(fsPathExists(pathTestFile) == true));
  printf("fsPathIsFile       6 : %s\n", PASSFAIL(fsPathIsFile(pathTestFile) == true));
  printf("fsPathIsDirectory  7 : %s\n", PASSFAIL(fsPathIsDirectory(pathTestFile) == false));
  printf("fsFileMove         8 : %s\n", PASSFAIL(fsFileMove(pathTestFile, "fileSystem.file.test", NULL, NULL) == true));
  printf("fsPathIsFile       9 : %s\n", PASSFAIL(fsPathIsFile(pathTestFile) == false));
  printf("fsPathIsFile      10 : %s\n", PASSFAIL(fsPathIsFile("fileSystem.file.test") == true));
  printf("fsFileMove        11 : %s\n", PASSFAIL(fsFileMove("fileSystem.file.test", pathTestFile, NULL, NULL) == true));
  printf("fsPathIsFile      12 : %s\n", PASSFAIL(fsPathIsFile(pathTestFile) == true));
  printf("fsPathIsFile      13 : %s\n", PASSFAIL(fsPathIsFile("fileSystem.file.test") == false));

  fsDirectoryDelete(pathTestDir, NULL, NULL); //don't care
  printf("fsPathExists      14 : %s\n", PASSFAIL(fsPathExists(pathTestDir) == false));
  printf("fsPathIsDirectory 15 : %s\n", PASSFAIL(fsPathIsDirectory(pathTestDir) == false));
  printf("fsDirectoryCreate 16 : %s\n", PASSFAIL(fsDirectoryCreate(pathTestDir, NULL, NULL) == true));
  printf("fsPathExists      17 : %s\n", PASSFAIL(fsPathExists(pathTestDir) == true));
  printf("fsPathIsDirectory 18 : %s\n", PASSFAIL(fsPathIsDirectory(pathTestDir) == true));
  printf("fsPathIsFile      19 : %s\n", PASSFAIL(fsPathIsFile(pathTestDir) == false));
  printf("fsDirectoryMove   20 : %s\n", PASSFAIL(fsDirectoryMove(pathTestDir, "fileSystem.dir.test", NULL, NULL) == true));
  printf("fsPathIsDirectory 21 : %s\n", PASSFAIL(fsPathIsDirectory(pathTestDir) == false));
  printf("fsPathIsDirectory 22 : %s\n", PASSFAIL(fsPathIsDirectory("fileSystem.dir.test") == true));
  printf("fsDirectoryMove   23 : %s\n", PASSFAIL(fsDirectoryMove("fileSystem.dir.test", pathTestDir, NULL, NULL) == true));
  printf("fsPathIsDirectory 24 : %s\n", PASSFAIL(fsPathIsDirectory(pathTestDir) == true));
  printf("fsPathIsDirectory 25 : %s\n", PASSFAIL(fsPathIsDirectory("fileSystem.dir.test") == false));

  char *cwd = fsPathCurrent(), *home = fsPathHome();
  printf("fsPathCurrent     26 : %s\n", cwd);
  printf("fsPathHome        27 : %s\n", home);
  char *base = fsPathBase(cwd), *parent = fsPathParent(cwd);
  printf("fsPathBase        28 : %s\n", base);
  printf("fsPathParent      29 : %s\n", parent);

  char *normal;
  normal = fsPathNormalize("test/bin");   printf("fsPathNormalize   30 : %s\n", normal); free(normal);
  normal = fsPathNormalize("./test/bin"); printf("fsPathNormalize   31 : %s\n", normal); free(normal);
  normal = fsPathNormalize("~/test/bin"); printf("fsPathNormalize   32 : %s\n", normal); free(normal);
  normal = fsPathNormalize("~");          printf("fsPathNormalize   33 : %s\n", PASSFAIL(strcmp(normal, home) == 0)); free(normal);
  normal = fsPathNormalize("/");          printf("fsPathNormalize   34 : %s\n", PASSFAIL(strcmp(normal, "/" ) == 0)); free(normal);
  normal = fsPathNormalize("/test1/test1/test1/.././../test2/./test3");
  printf("fsPathNormalize   35 : %s\n", PASSFAIL(strcmp(normal, "/test1/test2/test3") == 0)); free(normal);

  free(cwd); free(home); free(base); free(parent);

  char *rwTestData = "Hello\nfileSystem\r\nWord!";
  u32  rwTestLength = strlen(rwTestData);
  printf("fsFileSize        36 : %s\n", PASSFAIL(fsFileSize(pathTestFile) == 0));
  printf("fsFileWrite       37 : %s\n", PASSFAIL(fsFileWrite(pathTestFile, rwTestData, rwTestLength, NULL, NULL) == true));
  printf("fsFileSize        38 : %s\n", PASSFAIL(fsFileSize(pathTestFile) == rwTestLength));
  char *rwTestBuff = fsFileRead(pathTestFile, NULL, NULL);
  printf("fsFileRead        39 : %s\n", PASSFAIL(strncmp(rwTestData, rwTestBuff, rwTestLength) == 0));  
  free(rwTestBuff);

  printf("fsFileDelete      40 : %s\n", PASSFAIL(fsFileDelete(pathTestFile, NULL, NULL) == true));

  printf("fsDirectoryCreate 41 : %s\n", PASSFAIL(fsDirectoryCreate("fileSystem.test.dir/subA/subB", NULL, NULL) == true));
  printf("fsFileCreate      42 : %s\n", PASSFAIL(fsFileCreate("fileSystem.test.dir/file0", NULL, NULL) == true));
  printf("fsFileCreate      43 : %s\n", PASSFAIL(fsFileCreate("fileSystem.test.dir/file1", NULL, NULL) == true));

  list *dirStuff = fsDirectoryList(pathTestDir, FS_DIRLIST_DIRSFIRST, NULL);
  listItem *dirEntry = dirStuff->origin;
  for(u32 i=0; i<dirStuff->count; i++)
  {
    fsDirectoryItem *fsdi = (fsDirectoryItem *)dirEntry->data;
    printf("fsDirectoryList   %02d : (%c) %s\n", 44+i, (fsdi->isDirectory ? 'd' : 'f'), fsdi->name);
    dirEntry = dirEntry->next;
  }
  listFree(&dirStuff);

  printf("fsDirectoryDelete xx : %s\n", PASSFAIL(fsDirectoryDelete(pathTestDir, NULL, NULL) == true));

  return 0;  
}
