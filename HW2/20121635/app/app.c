#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>

#define FPGA_DEVICE "/dev/dev_driver"
#define IOCTL_SET_MSG _IOR(242,0,char *) 

int main(int argc, char **argv)
{

  int   ii;
  char  dev_interval;
  char  dev_count;
  short dev_option;

  int   option_check = 0;
  int   device;
  long  kernel_param;

  if( argc != 4){
    printf("please input the parameter! \n");
    printf("usage: [./app 1 14 0700] \n");
    return -1;
  }

  dev_interval  = atoi(argv[1]);
  dev_count     = atoi(argv[2]);
  dev_option    = atoi(argv[3]);

  // check parameter value;
  if( dev_interval < 1 || dev_interval > 100 ){
    printf("<<Error>> interval should be [1,100]\n ");
    return -1;
  }

  if( dev_count  < 1 || dev_count > 100 ){
    printf("<<Error>> Count should be [1,100]\n ");
    return -1;
  }

  if( strlen(argv[3]) != 4){
    printf("<<Error>> option's length should be 4\n");
    return -1;
  }
  else{
    for( ii = 0; ii < 4; ii++){
      if( argv[3][ii] < '0' || argv[3][ii] >'8' ){
        printf(" <<Error>> option's value is octal numeral\n");
        return -1;
      }
      if( argv[3][ii] == '0' )
        option_check++;
    }

    if( option_check == 4 ){
      printf(" <<Error>> option's value should not be zero[0000] \n");
      return -1;
    }

  }
  kernel_param = syscall(376, dev_interval, dev_count, dev_option);
  
  device = open(FPGA_DEVICE, O_WRONLY);
  if( device < 0){
    printf(" Device open error : %s\n", FPGA_DEVICE);
    exit(1);
  }

  ioctl(device, IOCTL_SET_MSG, kernel_param);

  close(device);
  return 0;

}


