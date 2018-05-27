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
#define IOM_FPGA_NAME 	"dev_driver"

#define IOM_FPGA_LED_ADDRESS 		0x08000016 	// pysical address
#define IOM_FPGA_FND_ADDRESS 		0x08000004 	// pysical address
#define IOM_FPGA_DOT_ADDRESS 		0x08000210 	// pysical address
#define IOM_FPGA_TEXT_ADDRESS 	0x08000090 	// pysical address - 32 Byte (16 * 2)

#define IOCTL_SET_MSG _IOR(IOM_FPGA_MAJOR, 0, char *)

static void dot_control(int init);
static void led_control(int init);
static void fnd_control(int init);
static void text_control(int init);
static void kernel_timer_blink(unsigned long timeout);

int iom_fpga_open(struct inode *minode, struct file *mfile);
int iom_fpga_release(struct inode *minode, struct file *mfile);
ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t iom_fpga_ioctl(struct file *file, unsigned int cmd, unsigned long kernel_param);

//global varibale
static int count      = 0;
static int interval   = 0;
static int option 	  = 0;
static int c_position = -1;
static int c_value    = -1;
static int s_position = -1;
static int s_value    = -1;

static int num_dir    = 1;
static int num_min    = -1;
static int num_max    = 0;

static int name_dir   = 1;
static int name_min   = -1;
static int name_max   = 0;

static int fpga_port_usage = 0;

static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_text_addr;

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

static void dot_control(int init)
{
  int ii;
  unsigned short int _s_value;

  // when exit(init)
  if( init == -1 ){
    for(ii = 0 ; ii < 10 ; ii++) {
      _s_value = 0;
      outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
    }
    return ;
  }

  for( ii = 0; ii < 10; ii ++){
    _s_value = fpga_number[c_value][ii] & 0x7f;
    outw(_s_value, (unsigned int)iom_fpga_dot_addr+ii*2);
  }
  
}


static void led_control(int init)
{
  unsigned short _s_value = 0;

  // when exit(init)
  if( init == -1){
    outw(_s_value, (unsigned int)iom_fpga_led_addr);
    return ;
  }


  if( c_value == 8 )
    _s_value = 1;
  else
    _s_value = 2 << (7 - c_value );

  outw(_s_value, (unsigned int)iom_fpga_led_addr);

}


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


static void text_control(int init)
{
  int ii;
  unsigned short int _s_value = 0;
  unsigned char value[33];

  char num[16]  = "20121635";
  char name[16] = "jang jong seok";
  memset( value , 0x00, sizeof(value)  ); // name

  // when exit.
  if( init == -1 ){

    for(ii = 0 ; ii < 33 ; ii++) {
      _s_value = (value[ii] & 0xFF) << 8 | (value[ii + 1] & 0xFF);
      outw(_s_value,(unsigned int)iom_fpga_text_addr+ii);
      ii++;
    }
    return ;
  }

  num_max   = num_min   + strlen(num);
  name_max  = name_min  + strlen(name);

  if( num_dir == 1){ // move right
    num_min++;
    num_max++;

    if( num_max == 16 )
      num_dir = 0;
  }
  else{
    num_min--;
    num_max--;
    if( num_min == 0 )
      num_dir = 1;
  }

  if( name_dir == 1){ // move right
    name_min++;
    name_max++;

    if( name_max == 16 )
      name_dir = 0;
  }
  else{
    name_min--;
    name_max--;
    if( name_min == 0 )
      name_dir = 1;
  }

  //top 
  for( ii = 0; ii < 16; ii++){
    if( ii >= num_min && ii <= num_max)
      value[ii] = num[ii-num_min];
  }

  //bottle
  for( ii = 0; ii < 16; ii++){
    if( ii >= name_min && ii < name_max)
      value[ii+16] = name[ii-name_min];
  }


  for( ii = 0; ii < 33; ii++){
    _s_value = (value[ii] & 0xFF) << 8 | (value[ii + 1] & 0xFF);
    outw(_s_value,(unsigned int)iom_fpga_text_addr+ii);
    ii++;
  }

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
    led_control(-1);
    dot_control(-1);
    text_control(-1);
    return;
    // exit
  }

  fnd_control(0);
  led_control(0);
  dot_control(0);
  text_control(0);

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

  iom_fpga_dot_addr   = ioremap(IOM_FPGA_DOT_ADDRESS, 	0x10);
  iom_fpga_fnd_addr   = ioremap(IOM_FPGA_FND_ADDRESS, 	0x4);
  iom_fpga_led_addr   = ioremap(IOM_FPGA_LED_ADDRESS, 	0x1);
  iom_fpga_text_addr  = ioremap(IOM_FPGA_TEXT_ADDRESS, 	0x32);

  init_timer(&(mydata.timer));

  printk("init module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);

  return 0;
}


void __exit iom_fpga_exit(void)
{


  iounmap(iom_fpga_dot_addr);
  iounmap(iom_fpga_fnd_addr);
  iounmap(iom_fpga_led_addr);
  iounmap(iom_fpga_text_addr);

  del_timer_sync(&mydata.timer);
  unregister_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME);

  printk("exit module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);
}


module_init(iom_fpga_init);
module_exit(iom_fpga_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JongSeokJang");

