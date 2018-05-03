#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

struct my_data{
	int a,b,c;
};

extern long newcall(struct my_data *block);

int calldev_init(void){
	struct my_data data;
	printk(KERN_INFO "Embedded SoftWare 2017\n");
	data.a=7;
	data.b=3;

	printk(KERN_INFO "sysc# = %ld\n", newcall(&data));

	printk(KERN_INFO "a + b = %d\n", data.a);
	printk(KERN_INFO "a - b = %d\n", data.b);
	printk(KERN_INFO "a %% b = %d\n", data.c);
	return 0;
}

void calldev_exit( void ) { }
module_init ( calldev_init );
module_exit ( calldev_exit );
MODULE_LICENSE("Dual BSD/GPL");
