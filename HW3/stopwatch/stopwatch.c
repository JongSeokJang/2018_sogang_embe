#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#define DEVICE_MAJOR  242
#define DEVICE_NAME   "stopwatch"

#define IOM_FPGA_FND_ADDRESS    0x08000004  // pysical address

static unsigned char *iom_fpga_fnd_addr;

static struct struct_mydata {
  struct timer_list timer;
  int count;
};

static struct struct_mydata mydata;

static struct cdev inter_cdev;
//static int inter_major=0, inter_minor=0;

static int start_flag = 0;
static int stop_flag = 0;
static int result;
//static dev_t inter_dev;

static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

static void kernel_timer_blink(unsigned long timeout);
static void fnd_control(void);
int exitFlag = 1;

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

static struct file_operations inter_fops =
{
  .open     = inter_open,
  .write    = inter_write,
  .release  = inter_release,
};

// HOME Button
// start 
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
  printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

  // not start;
  if( start_flag == 0 ){
    start_flag  = 1;
    stop_flag   = 0;
    add_timer(&mydata.timer);
  }
  // working now
  else {

    // stop_flag
    if( stop_flag == 1 ){
      stop_flag   = 0;
      add_timer(&mydata.timer);
    }
  }
  return IRQ_HANDLED;
}

// BACK Button
// stop
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
  printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));

  if( stop_flag == 0 ){
    stop_flag   = 1;
    del_timer_sync(&mydata.timer);
  }

  return IRQ_HANDLED;
}

// VOL+ Button
// restart
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
  printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

  stop_flag     = 0;
  start_flag    = 0;
  mydata.count  = 0;

  fnd_control();
  del_timer_sync(&mydata.timer);

  return IRQ_HANDLED;
}

// VOL- Button
// exit when this button during fressing 3 second
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
  printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));

  del_timer_sync(&mydata.timer);
  __wake_up(&wq_write, 1, 1, NULL);

  /*
  if( gpio_get_value(IMX_GPIO_NR(5, 14)) == 0 ){
    // when falling interrupt, check 3 second for exit module
    printk("falling..\n");
    exitFlag = 1;

  }
  else{
    printk("rising..\n");
    exitFlag = 0;
    
  }
  */
  return IRQ_HANDLED;

}

static void kernel_timer_blink(unsigned long timeout) {

  printk("kernel_timer_blink!\n");

  fnd_control();

  mydata.count += 1;
  mydata.timer.expires  = get_jiffies_64() + (HZ);
  mydata.timer.data     = (unsigned long)&mydata;
  mydata.timer.function = kernel_timer_blink;

  add_timer(&mydata.timer);

}

static void fnd_control(void){

  unsigned char value[4];
  unsigned short int value_short = 0;
  int min, sec;

  memset(value, 0x00, sizeof(value) );

  mydata.count  = mydata.count % 3600;
  min = mydata.count / 60;
  sec = mydata.count % 60;

  value[0] = min / 10;
  value[1] = min % 10;
  value[2] = sec / 10;
  value[3] = sec % 10;

  value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
  outw(value_short, (unsigned int)iom_fpga_fnd_addr);

}

static int inter_open(struct inode *minode, struct file *mfile){
  int ret;
  int irq;

  printk(KERN_ALERT "Open Module\n");

  // int1
  gpio_direction_input(IMX_GPIO_NR(1,11));
  irq = gpio_to_irq(IMX_GPIO_NR(1,11));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

  // int2
  gpio_direction_input(IMX_GPIO_NR(1,12));
  irq = gpio_to_irq(IMX_GPIO_NR(1,12));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

  // int3
  gpio_direction_input(IMX_GPIO_NR(2,15));
  irq = gpio_to_irq(IMX_GPIO_NR(2,15));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

  // int4
  gpio_direction_input(IMX_GPIO_NR(5,14));
  irq = gpio_to_irq(IMX_GPIO_NR(5,14));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING, "voldown", 0);

  return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
  free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
  free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
  free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
  free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

  printk(KERN_ALERT "Release Module\n");
  return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){

  // Todo process
  mydata.count = 0;
  mydata.timer.expires  = get_jiffies_64() + (HZ);
  mydata.timer.data     = (unsigned long)&mydata;
  mydata.timer.function = kernel_timer_blink;

  if( start_flag==0 ){
    printk("sleep on\n");
    interruptible_sleep_on(&wq_write);
  }

  printk("write\n");
  return 0;
}

/*
static int inter_register_cdev(void)
{

  printk("hihi0: %d\n",inter_major );
  int error;
  if(inter_major) {
    inter_dev = MKDEV(inter_major, inter_minor);
    error = register_chrdev_region(inter_dev,1,"stopwatch");
  }else{
    printk("hihi0-1: %d\n",inter_major );
    error = alloc_chrdev_region(&inter_dev,inter_minor,1,"stopwatch");
    printk("hihi0-2: %d\n",inter_major );
    inter_major = MAJOR(inter_dev);
  }
  printk("hihi1: %d\n",inter_major );

  if(error<0) {
    printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
    return result;
  }
  printk("hihi2: %d\n",inter_major );

  printk(KERN_ALERT "major number = %d\n", inter_major);
  cdev_init(&inter_cdev, &inter_fops);
  inter_cdev.owner = THIS_MODULE;
  inter_cdev.ops = &inter_fops;
  error = cdev_add(&inter_cdev, inter_dev, 1);
  if(error)
  {
    printk(KERN_NOTICE "inter Register Error %d\n", error);
  }
  return 0;
}
*/


static int __init inter_init(void) {
  int result;
  //if((result = inter_register_cdev()) < 0 )
    
  result = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &inter_fops);
 
  if(result < 0) {
    printk(KERN_WARNING"Can't get any major\n");
    return result;
  }

  iom_fpga_fnd_addr   = ioremap(IOM_FPGA_FND_ADDRESS,   0x4);
  init_timer(&(mydata.timer));


  printk(KERN_ALERT "Init Module Success \n");
  printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");

  return 0;
}

static void __exit inter_exit(void) {

  del_timer_sync(&mydata.timer);
  unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);

  /*
  cdev_del(&inter_cdev);
  unregister_chrdev_region(inter_dev, 1);
  */

  iounmap(iom_fpga_fnd_addr);

  printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);
MODULE_LICENSE("GPL");
