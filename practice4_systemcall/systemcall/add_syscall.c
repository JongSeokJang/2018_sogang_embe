#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

struct mystruct {
	int myvar;
};

int main(void){
	int a = 10;
	int b = 9;
	char c = '+';
	int i=syscall(376, 11);
	printf("syscall %d\n" ,i);

	struct mystruct my_st;
	my_st.myvar =999;
	
	int i2=syscall(377, &my_st);
	printf("syscall %d\n" ,i2);

	int i3;
	i3=syscall(378, a, b, c);
	printf("(%d %c %d )syscall %d\n", a,c,b,i3);

	c = '-';
	i3=syscall(378, a, b, c);
	printf("(%d %c %d )syscall %d\n", a,c,b,i3);

	c = '*';
	i3=syscall(378, a, b, c);
	printf("(%d %c %d )syscall %d\n", a,c,b,i3);

	c = '/';
	i3=syscall(378, a, b, c);
	printf("(%d %c %d )syscall %d\n", a,c,b,i3);

	return 0;
}
