#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <clib/string.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")

void stringExpect(string *str, char *expected, u32 test);
void stringTests();
void builderTests();

int main(int argc, char **argv)
{
  stringTests();
  printf("---------------\n");
  builderTests();
  return 0;
}

void stringTests()
{
  string *strA = stringFromCharacter('x', 32);
  stringExpect(strA, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0);
  stringFree(&strA);

  strA = stringFromCString("Hello Strings");
  stringExpect(strA, "Hello Strings", 1);
  stringFree(&strA);

  strA = stringFromNCString("StringLibStuffHere", 9);
  stringExpect(strA, "StringLib", 2);

  string *strB = stringDuplicate(strA);
  stringFree(&strA);
  stringExpect(strB, "StringLib", 3);
  stringFree(&strB);

  strA = stringCreate();
  stringExpect(strA, NULL, 4);

  stringAppendCharacter(strA, 'w', 3);       stringExpect(strA, "www"           , 5);
  stringAppendCString(strA, ".domain.");     stringExpect(strA, "www.domain."   , 6);
  stringAppendNCString(strA, "tldarxiv", 3); stringExpect(strA, "www.domain.tld", 7);
  strB = stringFromCString("/index.html \t ");
  stringAppendString(strA, strB);
  stringFree(&strB);
  stringExpect(strA, "www.domain.tld/index.html \t ", 8);

  stringTrim(strA, true, false); stringExpect(strA, "www.domain.tld/index.html \t ", 9);
  stringTrim(strA, true, true);  stringExpect(strA, "www.domain.tld/index.html", 10);
  stringUpperCase(strA);         stringExpect(strA, "WWW.DOMAIN.TLD/INDEX.HTML", 11);
  stringLowerCase(strA);         stringExpect(strA, "www.domain.tld/index.html", 12);

  stringClear(strA);
  stringExpect(strA, NULL, 13);

  stringAppendCString(strA, "0123456789");
  stringExpect(strA, "0123456789", 14);

  strB = stringSubstring(strA, 3, 4);
  stringExpect(strB, "3456", 15);
  stringFree(&strB);

  strB = stringSubstring(strA, 5, 0);
  stringExpect(strB, "56789", 16);
  stringFree(&strB);

  stringAssignCString(strA, "123|456!|789|");
  stringExpect(strA, "123|456!|789|", 17);

  u32 count;
  char separators[] = {'!','|','#'};
  string **stringArr = stringSplit(strA, separators, 3, &count);
  printf("Test 18  : %s\n", PASSFAIL(count == 5));
  stringExpect(stringArr[0], "123", 19);
  stringExpect(stringArr[1], "456", 20);
  stringExpect(stringArr[2], NULL , 21);
  stringExpect(stringArr[3], "789", 22);
  stringExpect(stringArr[4], NULL , 23);
  for(u32 i=0; i<count; i++) stringFree(&(stringArr[i]));
  free(stringArr);

  stringPrintF(strA, 4096, "%s %u!", "Test Format", 32);
  stringExpect(strA, "Test Format 32!", 24);

  char *cstr = stringToCString(strA);
  stringFree(&strA);
  printf("Test 25  : %s\n", PASSFAIL(strcmp(cstr, "Test Format 32!") == 0));
  free(cstr);
}

void builderTests()
{
  stringBuilder *sbA = stringBuilderCreate();
  stringBuilderAppendCharacter(sbA, 'w', 3);
  stringBuilderAppendCString(sbA, ".domain.");
  stringBuilderAppendNCString(sbA, "tldarxiv", 3);
  string *strA = stringFromCString("/index.html");
  stringBuilderAppendString(sbA, strA);
  stringFree(&strA);

  strA = stringBuilderToString(sbA);
  stringExpect(strA, "www.domain.tld/index.html", 0);
  stringFree(&strA);

  char *cstr = stringBuilderToCString(sbA);
  printf("Test  1  : %s\n", PASSFAIL(strcmp(cstr, "www.domain.tld/index.html") == 0));
  free(cstr);

  stringBuilderFree(&sbA);
}

void stringExpect(string *str, char *expected, u32 test)
{
  if(expected == NULL)
  {
    printf("Test %2ua : %s\n", test, PASSFAIL(str->length == 0));
    printf("Test %2ub : %s\n", test, PASSFAIL(str->data == NULL));
    return;
  }

  u32 length = (u32)strlen(expected);
  printf("Test %2ua : %s\n", test, PASSFAIL(str->length == length));
  printf("Test %2ub : %s\n", test, PASSFAIL(strncmp(str->data, expected, length) == 0));
}

