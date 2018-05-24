#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

int my_init_func(void){
		printk("start module\n");
		return 0;
}

void my_exit_func(void){
		printk("end module\n");
}

module_init( my_init_func );
module_exit( my_exit_func );
MODULE_LICENSE("Dual BSD/GPL");
