#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clib/fileSystem.h>

int main(int argc, char **argv)
{
  // our test file path
  char *testFile = fsPathNormalize("./clibFsTest");
  char *contW = "Hello fileSystem World!";

  // if this file already exists, delete it
  // (not worrying about success here)
  fsFileDelete(testFile, NULL, NULL);

  // create a file with our test content
  printf("Writing file %s\n", testFile);
  fsFileWrite(testFile, contW, strlen(contW), NULL, NULL);

  // make sure the file now exists
  if(fsPathIsFile(testFile))
    printf("File wrote okay.\n");
  else
    printf("Error writing file...\n");

  //delete the file
  fsFileDelete(testFile, NULL, NULL);

  // verify deletion occured
  if(fsPathIsFile(testFile))
    printf("Error deleting file...\n");
  else
    printf("File deleted okay.\n");

  // cleanup our path
  free(testFile);
  return 0;
}
