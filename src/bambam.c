#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void trim_start(char ** cp1, char ** cp2, int rem)
{
  printf("Untouched: %s\n", *cp1);
  *cp1 += rem;
  printf("Trimmed: %s\n", *cp1);
  return ;
}

void trim_end(char ** cp1, char ** cp2, int rem) 
{
  printf("Untouched: %s\n", *cp1);  
  // geht: *cp1 += rem;

  printf("Step1: %s\n", *cp1);  
  // geht: (*cp1)[0] = 0;
  *(*cp1 + rem) = 0;
  printf("Step2: %s\n", *cp1);  
  // geht: *cp1 -= rem;
  printf("Trimmed: %s\n", *cp1);
  return ;
}

int main(int argc, const char ** argv)
{

  char * str = (char*) malloc(21 * sizeof(char)); //"12345678901234567890";
  strncpy(str, "12345678901234567890", 20);
  printf("LEN=%i\n", strlen(*(&str)));

  str += 4;
  printf("Test: %s\n", str);
  str -= 4;
  printf("Test: %s\n", str);
  *(str + 5) = 'v';



  printf("Before: %s\n", str);
  trim_start(&str, NULL, 5);
  //trim_end(&str, NULL, 5);
  printf("After: %s\n", str);

  return 0;
}
