#include <linux/kernel.h>
#include <linux/uaccess.h>

struct mystruct {
	int myvar;
};

asmlinkage int sys_newcall2(struct mystruct *dd) {
	struct mystruct my_st;
	copy_from_user(&my_st, dd, sizeof(my_st));

	int val = my_st.myvar;
	printk("sys_newcall2 %d\n", val);

	return 22;
}
