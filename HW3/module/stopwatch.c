#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include "./fpga_dot_font.h"

#define IOM_FPGA_MAJOR 	242
#define IOM_FPGA_NAME 	"stopwatch"

#define IOM_FPGA_FND_ADDRESS 		0x08000004 	// pysical address

static void fnd_control(int init);
static void kernel_timer_blink(unsigned long timeout);

int iom_fpga_open(struct inode *minode, struct file *mfile);
int iom_fpga_release(struct inode *minode, struct file *mfile);
ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t iom_fpga_ioctl(struct file *file, unsigned int cmd, unsigned long kernel_param);

//global varibale
static int fpga_port_usage = 0;

static unsigned char *iom_fpga_fnd_addr;

static struct file_operations iom_fpga_fops =
{ 
  .open 	        = iom_fpga_open, 
  .write 	        = iom_fpga_write,
  .release        = iom_fpga_release,
  .unlocked_ioctl = iom_fpga_ioctl,
};

static struct struct_mydata {
  struct timer_list timer;
  int count;
};

struct struct_mydata mydata;

static void fnd_control(int init)
{
  unsigned char value[4];
  unsigned short int value_short = 0;

  memset( value, 0x00, sizeof(value) );

  // when exit(init)
  if(init == -1) {
    value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
    outw(value_short, (unsigned int)iom_fpga_fnd_addr);
    return ;
  }

  switch(c_position) {
    case 0:
      value[0] = c_value;
      break;
    case 1:
      value[1] = c_value;
      break;
    case 2:
      value[2] = c_value;
      break;
    case 3:
      value[3] = c_value;
      break;
  }

  //write value
  value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
  outw(value_short, (unsigned int)iom_fpga_fnd_addr);
}

// define functions...
int iom_fpga_open(struct inode *minode, struct file *mfile)
{
  if(fpga_port_usage != 0) return -EBUSY;
  fpga_port_usage = 1;
  return 0;

}

int iom_fpga_release(struct inode *minode, struct file *mfile)
{
  fpga_port_usage = 0;
  return 0;
}

ssize_t iom_fpga_ioctl(struct file *file, unsigned int cmd, unsigned long kernel_param)
{
  num_dir    = 1;
  num_min    = -1;
  num_max    = 0;

  name_dir   = 1;
  name_min   = -1;
  name_max   = 0;
  
  switch(cmd) {
    case IOCTL_SET_MSG:

      interval  = (kernel_param & 0xff000000) >> 24;
      count     = (kernel_param & 0x00ff0000) >> 16;
      option    = (kernel_param & 0x0000ffff);

      if( option % 10 != 0 ){
        s_position  = 3;
        s_value     = option % 10;
      }
      else if( (option/10)%10 != 0 ){
        s_position  = 2;
        s_value     = (option/10)%10;

      }
      else if( (option/100)%10 != 0 ){
        s_position  = 1;
        s_value     = (option/100)%10;
      }
      else if( (option/1000)%10 != 0 ){
        s_position  = 0;
        s_value     = (option/1000)%10;
      }
      c_position  = s_position;
      c_value     = s_value;
      printk("start value : [%d][%d]\n", c_position, c_value);

      del_timer_sync(&mydata.timer);

      mydata.count = count;
      mydata.timer.expires = get_jiffies_64() + (HZ/10);
      mydata.timer.data = (unsigned long)&mydata;
      mydata.timer.function   = kernel_timer_blink;

      add_timer(&mydata.timer);
      printk("write finished");
      break;
  }

  return 1;

}


ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
  num_dir    = 1;
  num_min    = -1;
  num_max    = 0;

  name_dir   = 1;
  name_min   = -1;
  name_max   = 0;
  // Toto this part
  const char *tmp = gdata;
  long kernel_timer_buff = 0;

  if ( copy_from_user(&kernel_timer_buff, tmp, 4) )
    return -EFAULT;

  interval  = (kernel_timer_buff & 0xFF000000) >> 24;
  count 	  = (kernel_timer_buff & 0x00FF0000) >> 16;
  option 	  = (kernel_timer_buff & 0x0000FFFF);

  if( option % 10 != 0 ){
    s_position  = 3;
    s_value     = option % 10;
  }
  else if( (option/10)%10 != 0 ){
    s_position  = 2;
    s_value     = (option/10)%10;

  }
  else if( (option/100)%10 != 0 ){
    s_position  = 1;
    s_value     = (option/100)%10;
  }
  else if( (option/1000)%10 != 0 ){
    s_position  = 0;
    s_value     = (option/1000)%10;
  }
  c_position  = s_position;
  c_value     = s_value;

  del_timer_sync(&mydata.timer);

  mydata.count = count;
  mydata.timer.expires = get_jiffies_64() + (HZ/10);
  mydata.timer.data = (unsigned long)&mydata;
  mydata.timer.function   = kernel_timer_blink;

  add_timer(&mydata.timer);
  printk("write finished");
  return 1;
}


// This function is Called by timer
static void kernel_timer_blink(unsigned long timeout) {

  // Toto this part

  if( mydata.count <= 0 ){
    fnd_control(-1);
    return;
    // exit
  }

  fnd_control(0);

  printk("\nkernel print, mydata.count : [%d]\n", mydata.count);

  c_value++;

  if( c_value == 9 )
    c_value = 1;

  if( c_value == s_value )
    c_position++;

  if( c_position == 4 )
    c_position = 0;


  mydata.count--;
  mydata.timer.expires = get_jiffies_64() + (interval * (HZ/10));
  mydata.timer.data = (unsigned long)&mydata;
  mydata.timer.function = kernel_timer_blink;

  add_timer(&mydata.timer);

}


int __init iom_fpga_init(void)
{
  int result;

  result = register_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME, &iom_fpga_fops);
  if(result < 0) {
    printk(KERN_WARNING"Can't get any major\n");
    return result;
  }

  iom_fpga_fnd_addr   = ioremap(IOM_FPGA_FND_ADDRESS, 	0x4);
  init_timer(&(mydata.timer));

  printk("init module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);

  return 0;
}


void __exit iom_fpga_exit(void)
{

  iounmap(iom_fpga_fnd_addr);

  del_timer_sync(&mydata.timer);
  unregister_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME);

  printk("exit module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);
}


module_init(iom_fpga_init);
module_exit(iom_fpga_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JongSeokJang");

