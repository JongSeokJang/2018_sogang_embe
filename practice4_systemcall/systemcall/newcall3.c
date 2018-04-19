#include <linux/kernel.h>
#include <linux/uaccess.h>

asmlinkage int sys_newcall3(int a, int b, char operator) {


	if( operator == '+' ){
		return a + b;
	}
	else if( operator == '-' ){
		return a - b;
	}
	else if( operator == '*' ){
		return a * b;
	}
	else if( operator == '/' ){
		return a / b;
	}
}
