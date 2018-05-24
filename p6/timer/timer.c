#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/slab.h>

struct timer_list *my_timer;
unsigned long data = 0;

void timer_handler(unsigned long arg){
	printk("timer tick : %ld\n", arg);
	my_timer->data++;
	my_timer->expires = get_jiffies_64() + HZ*1;
	add_timer(my_timer);
}

int kerneltimer_init_func(void){
	printk("start timer module\n");
	my_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if(my_timer==NULL)
		return -1;
	init_timer(my_timer);
	my_timer->expires = get_jiffies_64() + HZ*1;
	my_timer->data = data;
	my_timer->function = timer_handler;
	add_timer(my_timer);
	return 0;
}

void kerneltimer_exit_func(void){
	if(my_timer!=NULL){
		del_timer(my_timer);
		kfree(my_timer);
	}
	printk("end timer module\n");
}

module_init( kerneltimer_init_func );
module_exit( kerneltimer_exit_func );
MODULE_LICENSE("Dual BSD/GPL");
