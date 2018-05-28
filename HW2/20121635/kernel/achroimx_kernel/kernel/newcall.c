#include <linux/kernel.h>

asmlinkage long sys_newcall(char interval, char count, short startOption)
{
	long ret;
	ret = interval;
	ret = ret << 8;
	ret = ret | count;
	ret = ret << 16;
	ret = ret | startOption;
	return ret;
} 
