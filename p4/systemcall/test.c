#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

struct mystruct {
	int myvar;
};

int main(void){
	int i=syscall(376, 11);
	printf("syscall %d\n" ,i);

	struct mystruct my_st;
	my_st.myvar =999;
	
	int i2=syscall(377, &my_st);
	printf("syscall %d\n" ,i2);

	return 0;
}
