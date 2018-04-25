#include<stdio.h>

struct {
  char a;
  long long int   b;
  short c[2];
  short d;
  long long int   e;
} __attribute__((packed)) ttt;

struct {
  char www;
  char yyy;
  char zzz;
  short hhh;
  long long int lll;
} qqq;


int main(void){
  int l;

  printf("char size : [%d]\n", sizeof(char) );
  printf("short size : [%d]\n", sizeof(short) );
  printf("int size : [%d]\n", sizeof(int) );
  printf("int size : [%d]\n", sizeof(long long int) );


  l = sizeof(ttt);
  printf("%d\n", l);

  l = sizeof(qqq);
  printf("%d\n", l);

}
