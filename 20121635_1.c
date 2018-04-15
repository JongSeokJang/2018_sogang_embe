#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


#define FND_DEVICE              "/dev/fpga_fnd"
#define LED_DEVICE              "/dev/fpga_led"
#define SWITCH_DEVICE           "/dev/fpga_push_switch"
#define FPGA_TEXT_LCD_DEVICE    "/dev/fpga_text_lcd"
#define FPGA_DOT_DEVICE         "/dev/fpga_dot"
#define FPGA_BUZZER_DEVICE      "/dev/fpga_buzzer"


#define KEY_RELEASE     0
#define KEY_PRESS       1

#define SHM_KEY         1635
#define MEM_SIZE        1024

#define BUFF_SIZE       64

#define MAX_ROW         7
#define MAX_COL         10
#define MAX_STRING      32

int ChangeToOcta(int);
int ChangeToFour(int);
int ChangeToTwo(int);

void initDotshm(int*);

void input_process(int);
void output_process(int);
void main_process(int);

typedef struct __cursor {
  int row;
  int col;
}cursor;

char charTable [9][3] ={
    {'.','Q','Z'}, {'A','B','C'}, {'D','E','F'},
    {'G','H','I'}, {'J','K','L'}, {'M','N'.'O'},
    {'P','R','S'}, {'T','U','V'}, {'W','X','Y'}
};

char fpga_number[5][10] ={
    {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e},    // 1
    {0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},    // A
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	// initialize
    {0xff,0x41,0x55,0x41,0x5d,0xff,0x14,0x14,0x14,0x14},    // alarm
    {0xff,0x41,0x55,0x41,0x5d,0xff,0x22,0x22,0x22,0x22}     // alarm    
};

int main (int argc, char *argv[])
{
    int shm_id;
    int *shm_addr;

    

    shmi_id = shmget( (key_t)SHM_KEY, MEM_SIZE, IPC_CREATE|666);
    if( shm_id == -1){
        printf("shared memory access failed..\n");
        return -1;
    }

    switch( fork() ){
        case -1 :
            printf("fork failed\n");
            exit(-1);
             break;

        case 0 :
            input_process(shm_id);
            break;

        default :

            switch( fork() ){
            
                case -1 :
                    printf("fork failed\n");
                    exit(-1);
                    break;

                case 0  :
                    ouput_process(shm_id);

                default :
                    main_process(shm_id);
            }        

    }


}

void input_process(int shm_id)
{
    struct input_event ev[BUFF_SIZE];
    
    char *device    = "/dev/input/event0";              // READY device
    int *shm_addr   = shmat(shm_id, (char *)NULL, 0);

    int fd, rd, value, mode;
    
    shm_addr[0] = 1;    // mode
    memset(shm_addr, 0, MEM_SIZE);


    printf("input process %d started\n", getpid());

    if( (fd = open (device, O_RDONLY | O_NONBLOCK)) == -1 ) {
        printf ("%s is not a vaild device.n", device);
    }

    while(1){
        if ((rd = read (fd, ev, size * BUFF_SIZE)) == -1){
            continue;  
        }
        value = ev[0].value;

        if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Only read the key press event
            printf ("code%d\n", (ev[1].code));
        }

        if( value == KEY_RELEASE ) {
            printf ("key release\n");
        }
        else{
            if( value == KEY_PRESS){
                printf ("key press\n");
            }

            if(ev[0].code == 158) {  // BACK button
                shm_addr[0] = -1;
                break;
            }
            else if(ev[0].code == 115){     // VOL+ button

                mode = shm_addr[0];
                memset(shm_addr, 0x00, MEM_SIZE);

                //printf("before mem value : %d\n", shm_addr[0]);
                if( mode == 5 )
                    shm_addr[0] = 1;
                else
                    shm_addr[0]++;
                
                printf("vol+, Mode : %s\n", function[shm_addr[0]-1]);
                printf("input shared mem value : %d\n", shm_addr[0]);

            }
            else if(ev[0].code == 114){     // VOL- button

                mode = shm_addr[0];
                memset(shm_addr, 0x00, MEM_SIZE);

                if( mode[0] == 1 )
                    shm_addr[0] = 5;
                else
                    shm_addr[0]--;

                printf("vol- Mode : %s\n", function[shm_addr[0]-1]); 
                printf("input shared mem value : %d\n", shm_addr[0]);
            }
            printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
        }


        } 


    }
    printf("input process %d finished\n", getpid());
    return ;

}
void output_process(int shm_id)
{
    int *shm_addr = shmat(shm_id, (char *)NULL, 0);


    unsigned char lednum;
    unsigned char fnd_data[4];
    unsigned char ledtext[MAX_STRING];
    unsigned char retval;
    unsigned char buzzData;

    int fd, rd, mode;
    int exit_flag = 0;
     //for fnd device
    int dev_fnd;
    int dev_led;
    int dev_switch;
    int dev_text_lcd;
    int dev_dot_font;
    int dev_buzzer;

    printf("output process %d started\n", getpid());

    shm_addr[0]= 1;
    dev_fnd         = open(FND_DEVICE, O_RDWR);
    dev_led         = open(LED_DEVICE, O_RDWR);
    dev_text_lcd    = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
    dev_dot_font    = open(FPGA_DOT_DEVICE, O_WRONLY);
    dev_buzzer      = open(FPGA_BUZZER_DEVICE, O_RDWR);

    if(dev_fnd < 0) {
        printf("FND Device open Error..\n");
        return -1;
    }
    if(dev_led < 0) {
        printf("LED Device open Error..\n");
        return -1;
    }
    if(dev_text_lcd < 0) {
        printf("TEXT LCD Device open Error..\n");
        return -1;
    }
    if(dev_dot_font < 0) {
        printf("DOT FRONT Device open Error..\n");
        return -1;
    }
    if(dev_buzzer < 0) {
        printf("BUZZER Device open Error..\n");
        return -1;
    }


    while(1){
        exit_flag = 0;
        mode = shm_addr[0];
        switch(mode){
            case -1:
                memset( fnd_data,0x00, sizeof(fnd_data) );
                retval = write(dev_fnd, fnd_data, 4);
                if( retval < 0 ){
                    printf("FND write error \n");
                    return -1;
                }

                lednum = 0;
                retval = write(dev_led, &lednum, 1);
                if( retval < 0 ){
                    printf("LED write error \n");
                    return -1;
                }

                retval = write(dev_dot_font, fpga_number[2], sizeof(fpga_number[2]));
                if(retval < 0) {
                    printf("DOT write error..\n");
                    return -1;
                }                

                memset( ledtext, 0x00, sizeof(ledtext));
                retval = write(dev_text_lcd, ledtext, sizeof(ledtext) );
                if(retval < 0) {
                    printf("TEXT_LED write error..\n");
                    return -1;
                }


                buzzData = 2;
                retval = write(dev_buzzer, &buzzData, 1);
                if(retval < 0) {
                    printf("BUZZER write error..\n");
                    return -1;
                }

                printf("exit................!!\n");
                exit_flag = 1;

                break;
            case 1:

                retval = write(dev_dot_font, fpga_number[2], sizeof(fpga_number[2]));
                if(retval < 0) {
                    printf("DOT init error..\n");
                    return -1;
                }
                memset( ledtext, 0x00, sizeof(ledtext));
                retval = write(dev_text_lcd, ledtext, sizeof(ledtext) );
                if(retval < 0) {
                    printf("TEXT_LED write error..\n");
                    return -1;
                }

                initDotshm(shm_addr);

                fnd_data[0] = shm_addr[1];
                fnd_data[1] = shm_addr[2];
                fnd_data[2] = shm_addr[3];
                fnd_data[3] = shm_addr[4];

                retval = write(dev_fnd, &data, 4);
                if(retval < 0) {
                    printf("write error!!\n");
                    return -1;
                }

                lednum = shm_addr[6];
                retval = write(dev_led, &lednum, 1);	
                if(retval < 0) {
                    printf("write error!!\n");
                    return -1;
                }

                break;
            case 2:
            
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
            default:
                break;
        
        }
        if(exit_flag){
            break;
        } 

    }
    

    



}
void main_process(int shm_id)
{
    unsigned char push_sw_buff[MAX_BUTTON];
    int exit_flag = 0;
    cursor curdot;
    int idxCount[0] = {0};
    time_t ctime;
    struct tm *tm;
    int *shm_addr = shmat(shm_id, (char *)NULL, 0);

    int curStrnum   = 0;
    int pushcount   = 0;
    int pushcount2  = 0;
    int tmpcount    = 0;
    int buff_size   = 0;
    int addhour     = 0;
    int addmin      = 0;
    int add10hour   = 0;
    int add10min    = 0;
    int countType   = 0;


    shm_addr[0] = 1;
    buff_size = sizeof(push_sw_buff);

    //open device
    dev_switch = open(SWITCH_DEVICE, O_RDWR);

    if(dev_switch < 0) {
        printf("SWITCH Device open Error..\n");
        return -1;
    }

    while(1){
        time(&ctime);
        tm=localtime(&ctime);

        switch( shm_addr[0]){
            case -1:
                exit_flag = 1;
                break;
            case 1:

                shm_addr[44] = tm->tm_sec;

                memset( idxCount, 0x00, sizeof(idxCount) );
                
                curStrnum = 0;
                pushcount = 0;
                pushcount2 = 0;
                tmpcount = 0;
                curdot.row = 6;
                curdot.col = 0;
                countType = 0;

                if(tm->tm_hour + addhour > 23) {
                    addhour = addhour - 24;
                }
                if(tm->tm_min + addmin > 59) {
                    addmin = addmin - 60;
                    addhour += 1;
                }
                //get board time
                tmphour = tm->tm_hour + addhour;
                tmpmin = tm->tm_min + addmin;


                //calculate time
                cur10hour = tmphour/10;
                curhour = tmphour%10;
                cur10min = tmpmin/10;
                curmin = tmpmin%10;

                //save time at shared memory
                shm_addr[1] = cur10hour; 
                shm_addr[2] = curhour;  
                shm_addr[3] = cur10min;
                shm_addr[4] = curmin;

                //Mode change and led contorl

                read(dev_switch, &push_sw_buff, buff_size);			

                if(push_sw_buff[0] == 1 && shm_addr[5] == 0) {
                    shm_addr[5] = 1;
                    printf("chage mode start....\n");
                }
                else if(push_sw_buff[0] == 1 && shm_addr[5] == 1) {
                    shm_addr[5] = 0;
                    printf("change mode finished\n");
                }

                if(shm_addr[5] == 1) {
                    ////for led
                    if(shm_addr[44] % 2 == 0) {
                        shm_addr[6] = 32;
                    }
                    else {
                        shm_addr[6] = 16;
                    }

                    ///for switch

                    if(push_sw_buff[1] == 1) {
                        addmin = 0;
                        addhour = 0;
                    }
                    else if(push_sw_buff[2] == 1) {
                        addhour += 1;
                    }
                    else if(push_sw_buff[3] == 1) {
                        addmin += 1;
                    }

                }
                else {
                    shm_addr[6] = 128;
                }

                break;

            case 2:
            case 3:
            case 4:
            case 5:
            default:

            if( exit_flag ){
                break;
            }

        }

    }



}



void initDotshm(int* shm_addr) {   //initialize dot data
  int i;
  shm_addr[42] = 6;
  shm_addr[43] = 0;
  for(i = 45 ; i < 115 ; i++) {
    shm_addr[i] =0;
  }
}



int ChangeToOcta(int num)
{
    if( num < 8 ){
        return num;
    }
    else{
        return ChangeToOcta(num/8)*10 + num%8;
    }
}
int ChangeToFour(int num)
{
    if( num < 4 ){
        return num;
    }
    else{
        return ChangeToFour(num/4)*10 + num%4;
    }

}
int ChangeToTwo(int num)
{
    if( num < 2 ){
        return num;
    }
    else{
        return ChangeToTwo(num/2)*10 + num%2l
    }

}
