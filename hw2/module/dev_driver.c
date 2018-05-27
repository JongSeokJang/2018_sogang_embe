#include <linux/module.h>
#include <linux/fs.h>
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


#define IOM_LED_ADDRESS 		0x08000016 	// pysical address
#define IOM_FND_ADDRESS 		0x08000004 	// pysical address
#define IOM_FPGA_DOT_ADDRESS 		0x08000210 	// pysical address
#define IOM_FPGA_TEXT_LCD_ADDRESS 	0x08000090 	// pysical address - 32 Byte (16 * 2)



static void dot_control(int init);
static void led_control(int init);
static void fnd_control(int init);
static void text_control(int init);


int iom_fpga_open(struct inode *minode, struct file *mfile);
int iom_fpga_release(struct inode *minode, struct file *mfile);
ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
static void kernel_timer_blink(unsigned long timeout);

//global varibale
static int inteval 	    = 0;
static int option 	    = 0;
static int s_position   = -1;
static int c_position   = -1;
static int s_val        = -1;
static int c_val        = -1;

static int fpga_port_usage = 0;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_text_lcd_addr;

static struct file_operations iom_fpag_fops =
{ 
  .open 	 = iom_fpga_open, 
  .write 	 = iom_fpga_write,
  .release = iom_fpga_release
};

static struct struct_mydata {
  struct timer_list timer;
  int count;
};

struct struct_mydata mydata;

static void dot_control(int init)
{
  int i;
  unsigned short int _s_value;

  //when exit module initialize dot value to 0
  if(init == 1) {
    for(i = 0 ; i < 10 ; i++) {
      _s_value = 0;
      outw(_s_value, (unsigned int)iom_fpga_dot_addr+i*2);
    }
    return ;
  }
  //write value range 1~8
  if(number == 9)
    number = 1;
  //write value
  for(i = 0 ; i < 10 ; i++) {
    _s_value = fpga_number[number][i] & 0x7f;
    outw(_s_value, (unsigned int)iom_fpga_dot_addr+i*2);
  }

}
static void led_control(int init)
{
  unsigned short _s_value = 1;
  int mul = 2;
  int i;
  int tmpnum = 8 - number;
  //when exit module initialize led
    if(init == 1) {
      _s_value = 0;
      outw(_s_value, (unsigned int)iom_fpga_led_addr);
      return ;
    }

  //write to led device
  for(i = 0 ; i < tmpnum ; i++)
    _s_value = _s_value * mul;

  printk("led value : %d\n", _s_value);

  outw(_s_value, (unsigned int)iom_fpga_led_addr);


}
static void fnd_control(int init)
{
  unsigned short int value_short = 0;
  unsigned char value[4];

  value[0] = 0;
  value[1] = 0;
  value[2] = 0;
  value[3] = 0;

  //when exit module initialize fnd
  if(init == 1) {
    value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
    outw(value_short, (unsigned int)iom_fpga_fnd_addr);
    return ;
  }

  //write for range 1~8
  if(number == 9)
    number = 1;

  if(loc < 0)
    loc = 3;


  if(loc == 1) {
    value[3] = number;
  }
  else if(loc == 2) {
    value[2] = number;
  }
  else if(loc == 3) {
    value[1] = number;
  }
  else {
    value[0] = number;
  }

  //write value
  value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
  outw(value_short, (unsigned int)iom_fpga_fnd_addr);



}
static void text_control(int init)
{
  int i;
  unsigned short int _s_value = 0;
  unsigned char value[33];

  //string value
  char num[10] = "20121635";
  char name[15] = "Jang Jong Seok";
  int space1;
  int space2;
  //initialize when exit
  if(init == 1) {
    for(i = 0 ; i < 33 ; i++)
      value[i] = 0;

    for(i = 0 ; i < 33 ; i++) {
      _s_value = (value[i] & 0xFF) << 8 | (value[i + 1] & 0xFF);
      outw(_s_value,(unsigned int)iom_fpga_text_lcd_addr+i);
      i++;
    }


    return ;
  }

  //make string
  space1 = numL - 0;
  space2 = nameL - numR - 1;

  for(i = 0 ; i < space1 ; i++)
    value[i] = ' ';

  for(i = space1 ; i < numR + 1 ; i++)
    value[i] = num[i - space1];

  for(i = numR + 1 ; i < nameL ; i++)
    value[i] = ' ';

  for(i = nameL ; i < nameR + 1 ; i++)
    value[i] = name[i - nameL];

  for(i = nameR + 1 ; i < 32 ; i++)
    value[i] = ' ';
  value[32] = '\0';



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

ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
  // Toto this part
  const char *tmp = gdata;
  long kernel_timer_buff = 0;

  if ( copy_from_user(&kernel_timer_buff, tmp, 4) )
    return -EFAULT;

  inteval = (kernel_timer_buff & 0xFF000000) >> 24;
  count 	= (kernel_timer_buff & 0x00FF0000) >> 16;
  option 	= (kernel_timer_buff & 0x0000FFFF);

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
    s_value     = (option/100)%10;
  }
  c_position  = s_position;
  c_value     = c_value;

  del_timer_sync(&mydata.timer);

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

  // exit module
  if(mydata.count == 0 ){
    dot_control(0);
    fnd_control(0);
    led_control(0);
    lext_lcd_control(0);
    return;
  }
  // initalize module

  dot_control(1);
  led_control(1);
  fnd_control(1);
  text_control(1);

  c_val++;
  if( c_val > 8 ){

    c_val = 1;
    if( c_val < s_val)  // why?
      c_val = 1
    else{
      c_position++;
      if( c_position > 8 )
        c_position = 0;
    }
  }
  else if( c_val == s_val){
    c_position++;
    if( c_position > 3)
      c_position = 0;
  }


  mydata.count--;
  mydata.timer.expires = get_jiffies_64() + (interval * (HZ/10));
  mydata.timer.data = (unsigned long)&mydata;
  mydata.timer.function = kernel_timer_blink;

  add_timer(&mydata.timer);

}



int __init iom_fpga_init(void)
{
  int result;

  //struct class *kernel_timer_dev_class 	= NULL;
  //struct device *kernel_timerdev 		= NULL;

  result = register_chrdev(IOM_FPGA_MAJOR, IOM_FPGA__NAME, &iom_fpga_dot_fops);
  if(result < 0) {
    printk(KERN_WARNING"Can't get any major\n");
    return result;
  }


  iom_fpga_dot_addr 	= ioremap(IOM_FPGA_DOT_ADDRESS, 	0x10);
  iom_fpga_fnd_addr 	= ioremap(IOM_FND_ADDRESS, 		0x4);
  iom_fpga_led_addr 	= ioremap(IOM_LED_ADDRESS, 		0x1);
  iom_fpga_text_lcd__addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 	0x32);


  //kernel_timer_dev = device_create(kernel_timer_dev_class, NULL, MKDEV(IOM_FPGA_MAJOR,0), NULL, IOM_FPGA_NAME);

  init_timer(&(mydata.timer));


  printk("init module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);

  return 0;
}

void __exit iom_fpga_dot_exit(void)
{
  // fpga_dot
  iounmap(iom_fpga_dot_addr);

  // fpga_fnd
  iounmap(iom_fpga_fnd_addr);

  // fpga led
  iounmap(iom_fpga_led_addr);

  // fpga text led
  iounmap(iom_fpga_text_led_addr);
  //timer
  del_timer_sync(&mydata.timer);

  unregister_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME);


}


module_init(iom_fpga_init);
module_exit(iom_fpga_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");

