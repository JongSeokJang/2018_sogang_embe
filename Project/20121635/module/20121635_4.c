#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#include "./fpga_dot_font.h"

#define DEVICE_MAJOR  242
#define DEVICE_NAME   "dev_driver"

#define IOM_FPGA_FND_ADDRESS    0x08000004  // pysical address
#define IOM_FPGA_DOT_ADDRESS    0x08000210  // pysical address

static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_dot_addr;

static struct struct_mydata {
  struct timer_list timer;
  struct timer_list exit_timer;
  int count;
};

static struct struct_mydata mydata;


static int mode           = 1;
static int timer_set_flag = 0;
static int button_count   = 0;

static int result;

static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);

static void kernel_timer_blink(unsigned long timeout);
static void exit_timer_blink(unsigned long timeout);

static void fnd_control(void);
static void dot_control(int status);
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

  if( mode == 1 ){
    mode = 2;
    printk("mode : [%d]\n", mode);

    timer_set_flag = 0;
    mydata.count = 0;
    fnd_control();
    dot_control(-1);

  }
  else{
    mode = 1;
    printk("mode : [%d]\n", mode);

    fnd_control();
    dot_control(0);

  }

  return IRQ_HANDLED;
}

// VOL+ Button
// restart
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
  printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

  if( mode == 2 ){
    if( timer_set_flag == 0 ){
      // current time plus 1 seccond
      mydata.count = (mydata.count+1)%61;
      fnd_control();
    }
  }

  return IRQ_HANDLED;
}

// VOL- Button
// exit when this button during fressing 3 second
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {

  printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));

  if( mode == 2 ){
    if( timer_set_flag == 0 ){
      timer_set_flag = 1;

      mydata.timer.expires  = get_jiffies_64() + (HZ);
      mydata.timer.data     = (unsigned long)&mydata;
      mydata.timer.function = kernel_timer_blink;
      add_timer(&mydata.timer);
    }

  }
  
  return IRQ_HANDLED;
}


static void kernel_timer_blink(unsigned long timeout) {

  printk("kernel_timer_blink!\n");

  fnd_control();
  dot_control(0);

  if( mydata.count != 0 ){
    mydata.count -= 1;
    mydata.timer.expires  = get_jiffies_64() + (HZ);
    mydata.timer.data     = (unsigned long)&mydata;
    mydata.timer.function = kernel_timer_blink;
    add_timer(&mydata.timer);
  }
}

static void exit_timer_blink(unsigned long timeout){

  printk("exit_timer_blink1!\n");

  del_timer_sync(&mydata.timer);

  mydata.count = 0;
  fnd_control();

  printk("exit_timer_blink2!\n");

  __wake_up(&wq_write,1,1,NULL);

}

static void dot_control(int status){

  unsigned short int _s_value;
  int ii;
  int c_value;

  if( status == -1 ){ // init
    printk("dot_control init status : %d\n", status);
    for( ii = 0; ii < 10; ii ++){
      _s_value = fpga_set_blank[ii] & 0x7f;
      outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
    }
  }
  else{

    printk("hihi");
    if( mode == 1 ){
      printk("hihi2");

      c_value = button_count%10;

      for( ii = 0; ii < 10; ii ++){
        _s_value = fpga_number[c_value][ii] & 0x7f;
        outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
      }
    }
    else if( mode == 2 ){

      if( mydata.count < 10 ){

        c_value = mydata.count;
        for( ii = 0; ii < 10; ii ++){
          _s_value = fpga_number[c_value][ii] & 0x7f;
          outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
        }

      }
      else{
        for( ii = 0; ii < 10; ii ++){
          _s_value = fpga_set_blank[ii] & 0x7f;
          outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
        }
      }
    }
  }
}

static void fnd_control(void){

  unsigned char value[4];
  unsigned short int value_short = 0;
  int min, sec;

  memset(value, 0x00, sizeof(value) );

  if( mode == 1 ){
    printk("fnd_control/ mode:1 button_count[%d]\n", button_count);
    value[0] = button_count/1000;
    value[1] = (button_count%1000)/100;
    value[2] = (button_count%100)/10;
    value[3] = button_count%10;
  }

  else if( mode == 2 ){
    value[0] = 0;
    value[1] = 0;
    value[2] = mydata.count/10;
    value[3] = mydata.count%10;
  }

  value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
  outw(value_short, (unsigned int)iom_fpga_fnd_addr);

}

static int inter_open(struct inode *minode, struct file *mfile){
  int ret;
  int irq;

  mode           = 1;
  timer_set_flag = 0;
  button_count   = 0;
  fnd_control();
  dot_control(0);

  printk(KERN_ALERT "Open Module\n");

  // int1
  gpio_direction_input(IMX_GPIO_NR(1,11));
  irq = gpio_to_irq(IMX_GPIO_NR(1,11));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);


  // int3
  gpio_direction_input(IMX_GPIO_NR(2,15));
  irq = gpio_to_irq(IMX_GPIO_NR(2,15));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

  // int4
  gpio_direction_input(IMX_GPIO_NR(5,14));
  irq = gpio_to_irq(IMX_GPIO_NR(5,14));
  printk(KERN_ALERT "IRQ Number : %d\n",irq);
  ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "voldown", 0);

  return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){

  mode           = 1;
  timer_set_flag = 0;
  button_count   = 0;
  fnd_control();
  dot_control(-1);

  free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
  free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
  free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

  del_timer_sync(&mydata.timer);
  printk(KERN_ALERT "Release Module\n");
  return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){


  if(count == 200){ 

    if( mode == 1 ){

      printk("write \n");
      button_count++;
      fnd_control();
      dot_control(0);

      printk("write\n");
      return 1;
    }

  }
  else if( count == 100 ){

    printk("inter_write count : 100/ timer_set_flag : [%d]\n", timer_set_flag);
    if( timer_set_flag == 1){

      return mydata.count;
    }
    else{
      return 1;
    }

  }

}


static int __init inter_init(void) {
  int result;
    
  result = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &inter_fops);
 
  if(result < 0) {
    printk(KERN_WARNING"Can't get any major\n");
    return result;
  }

  iom_fpga_fnd_addr   = ioremap(IOM_FPGA_FND_ADDRESS,   0x4);
  iom_fpga_dot_addr   = ioremap(IOM_FPGA_DOT_ADDRESS,   0x10);

  init_timer(&(mydata.timer));

  printk(KERN_ALERT "Init Module Success \n");
  printk(KERN_ALERT "Device : /dev/dev_driver, Major Num : 242 \n");

  return 0;
}

static void __exit inter_exit(void) {

  mode           = 1;
  timer_set_flag = 0;
  button_count   = 0;

  del_timer_sync(&mydata.timer);

  unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
  iounmap(iom_fpga_fnd_addr);

  printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);
MODULE_LICENSE("GPL");
