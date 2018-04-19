#include <linux/kernel.h>

asmlinkage int sys_newcall(int a) {
	printk("sys_newcall %d\n", a);

	return 21;
}
