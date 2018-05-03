#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

struct my_data{
	int a,b,c;
};

struct my_new_function_data{
	int length;
	int data[10];
}

extern long newcall(struct my_data *block);
extern void mynew_function(struct my_new_function_data *numInfo, int num);

int calldev_init(void){
	int ii;
	int num = 1234;
	struct my_data data;
	struct my_new_function_data newData;
	printk(KERN_INFO "Embedded SoftWare 2017\n");
	data.a=7;
	data.b=3;

	printk(KERN_INFO "sysc# = %ld\n", newcall(&data));

	printk(KERN_INFO "a + b = %d\n", data.a);
	printk(KERN_INFO "a - b = %d\n", data.b);
	printk(KERN_INFO "a %% b = %d\n", data.c);



	mynew_function(&newData, num);
	printk(KERN_INFO, "length : %d", newData.length);

	for( ii = 0; ii < newData.length; ii++){
		printk(KERN_INFO, "arr[%d] : %d\n", ii, newData.data[ii]);
	}
	

	return 0;
}

void calldev_exit( void ) { }
module_init ( calldev_init );
module_exit ( calldev_exit );
MODULE_LICENSE("Dual BSD/GPL");
