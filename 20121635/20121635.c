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


// driver path
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
#define MAX_BUTTON      9

void initDotshm(int*);

int input_process(int);
int output_process(int);
int main_process(int);

int changeToTwo(int);
int changeToFour(int);
int changeToOcta(int);

void makeString(int* , int , char );
void writeString(int *, unsigned char* );
void countPush(int *shm_addr, int pushcount);

void makeDotAry(int* shm_addr, char fpga[10]);
void inverseDot(int* shm_addr);
void refreshDot(int* shm_addr);


int makeAnswer(int, int, int);
void makeNumber(int *, int *);
int makeOperator(int, int);


int ppow(int num, int mul);
int makeIdx(int row, int col);

typedef struct __cursor {
  int row;
  int col;
}cursor;

char charTable [9][3] ={
    {'.','Q','Z'}, {'A','B','C'}, {'D','E','F'},
    {'G','H','I'}, {'J','K','L'}, {'M','N','O'},
    {'P','R','S'}, {'T','U','V'}, {'W','X','Y'}
};

char fpga_number[3][10] ={
    {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e},    // 1
    {0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},    // A
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}		// initialize
};

char fpga_number_1[3][10] = {	// for alarm
	{0x63,0x63,0x63,0x7F,0x7F,0x63,0x63,0x7F,0x60,0x60},
	{0x1c,0x1c,0x1c,0x00,0x00,0x1c,0x1c,0x00,0x1F,0x1F},
	{0x08,0x1c,0x1c,0x3E,0x3E,0x7F,0x7F,0x08,0x08,0x1C}
};


int main (int argc, char *argv[])
{
    int shm_id;
    int *shm_addr;
    int retval;

    shm_id = shmget( (key_t)SHM_KEY, MEM_SIZE, IPC_CREAT|666);
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
            retval = input_process(shm_id);
            break;

        default :

            switch( fork() ){
            
                case -1 :
                    printf("fork failed\n");
                    exit(-1);
                    break;

                case 0  :
                    retval = output_process(shm_id);

                default :
                    retval = main_process(shm_id);
            }        

    }
	return 0;

}


/* input_process */
int input_process(int shm_id)
{
    struct input_event ev[BUFF_SIZE];

    
    char *device    = "/dev/input/event0";              	// READY device
    int *shm_addr   = shmat(shm_id, (char *)NULL, 0);

    int fd, rd, value, mode;
    int size = sizeof(struct input_event);
    
    shm_addr[0] = 1;    // mode
    memset(shm_addr, 0, MEM_SIZE); // init

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
				exit(1);
                break;
            }
            else if(ev[0].code == 115){     // VOL+ button

                mode = shm_addr[0];
                memset(shm_addr, 0x00, MEM_SIZE);

                if( mode == 5 )
                    shm_addr[0] = 1;
                else
                    shm_addr[0] = mode + 1;
                
                printf("input shared mem value : %d\n", shm_addr[0]);

            }
            else if(ev[0].code == 114){     // VOL- button

                mode = shm_addr[0];
                memset(shm_addr, 0x00, MEM_SIZE);

                if( mode == 1 )
                    shm_addr[0] = 5;
                else
                    shm_addr[0] = mode -1;

                printf("input shared mem value : %d\n", shm_addr[0]);
            }
            printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
        }

    } 

    printf("input process %d finished\n", getpid());
    return ;

}
int output_process(int shm_id)
{
    int *shm_addr = shmat(shm_id, (char *)NULL, 0);

    unsigned char lednum;
    unsigned char fnd_data[4];
    unsigned char ledtext[MAX_STRING];
    unsigned char retval;
    unsigned char buzzData;

    char fpga_data[10] = {0};

    int fd, rd, mode;
    int exit_flag = 0;
	int back_flag = 0;

    int dev_fnd;
    int dev_led;
    int dev_text_lcd;
    int dev_dot_font;
    int dev_buzzer;


	int set_num;
	int row, col;

    int i, j;

    int check;

    printf("output process %d started\n", getpid());

    shm_addr[0] = 1;
    dev_fnd         = open(FND_DEVICE, 				O_RDWR);
    dev_led         = open(LED_DEVICE, 				O_RDWR);
	dev_buzzer      = open(FPGA_BUZZER_DEVICE, 		O_RDWR);
    dev_dot_font    = open(FPGA_DOT_DEVICE, 		O_WRONLY);
    dev_text_lcd    = open(FPGA_TEXT_LCD_DEVICE, 	O_WRONLY);


	// check driver file open error
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


			// back_buttion 
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
				back_flag = 1;

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

                retval = write(dev_fnd, &fnd_data, 4);
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
            

                retval = write(dev_dot_font, fpga_number[2], sizeof(fpga_number[2]));
                if(retval < 0) {
                    printf("DOT init error..\n");
                    return -1;
                }

                memset(ledtext, 0x00, MAX_STRING);
                retval = write(dev_text_lcd, ledtext, MAX_STRING);

                fnd_data[0] = 0;
                fnd_data[1] = shm_addr[2];
                fnd_data[2] = shm_addr[3];
                fnd_data[3] = shm_addr[4];

                retval = write(dev_fnd, &fnd_data, 4);
                if(retval < 0) {
                    printf("write error..\n");
                    return -1;
                }

                lednum = shm_addr[6];
                retval = write(dev_led, &lednum, 1);
                if(retval < 0) {
                    printf("write error!!\n");
                    return -1;
                }


                break;


            case 3:

                //led initialize
                lednum = 0;
                retval = write(dev_led, &lednum, 1);
                if(retval < 0) {
                    printf("write error!!\n");
                    return -1;
                }	
                //init dot
                initDotshm(shm_addr);

                if(shm_addr[7] == 0) 
                    set_num = 1;
                else 
                    set_num  = 0;

                retval = write(dev_dot_font, fpga_number[set_num], sizeof(fpga_number[set_num]));
                if(retval < 0) {
                    printf("DOT write error..\n");
                    return -1;
                }

                //data for fnd counter
                fnd_data[0] = shm_addr[1];
                fnd_data[1] = shm_addr[2];
                fnd_data[2] = shm_addr[3];
                fnd_data[3] = shm_addr[4];
                retval = write(dev_fnd, &fnd_data, 4);
                if(retval < 0) {
                    printf("FND write error..\n");
                    return -1;
                }


                //write to text lcd
                writeString(shm_addr, ledtext);

                retval = write(dev_text_lcd, ledtext, MAX_STRING);
                if(retval < 0) {
                    printf("TEXT_LCD write error..\n");
                    return -1;
                }

                break;


            case 4:


                lednum = 0;
                retval = write(dev_led, &lednum, 1);
                if(retval < 0) {
                    printf("LED init error..\n");
                    return -1;
                }
                retval = write(dev_dot_font, fpga_number[2], sizeof(fpga_number[2]));
				
                if(retval < 0) {
                    printf("DOT init error..\n");
                    return -1;
                }
				
                //init text lcd
                memset(ledtext, 0x00, MAX_STRING);
                retval = write(dev_text_lcd, ledtext, MAX_STRING);


				memset(fpga_data, 0x00, sizeof(fpga_data) );

                //write precedent image
                makeDotAry(shm_addr, fpga_data);

                /////write cursor
                row = shm_addr[42];
                col = shm_addr[43];

                if(shm_addr[44] % 2 == 0 && shm_addr[makeIdx(row, col)] == 0 && shm_addr[115] == 0) {
                    fpga_data[col] += ppow(2, row);
                    retval = write(dev_dot_font, fpga_data, sizeof(fpga_data));
                }

                write(dev_dot_font, fpga_data, sizeof(fpga_data));

                //write fnd...
                fnd_data[0] = shm_addr[1];
                fnd_data[1] = shm_addr[2];
                fnd_data[2] = shm_addr[3];
                fnd_data[3] = shm_addr[4];

                retval = write(dev_fnd, &fnd_data, 4);
                if(retval < 0) {
                    printf("FND write error..\n");
                    return -1;
                }
                break;


            case 5:

                //save current data
                fnd_data[0] = shm_addr[1];
                fnd_data[1] = shm_addr[2];  
                fnd_data[2] = shm_addr[3];
                fnd_data[3] = shm_addr[4];

                retval = write(dev_fnd, &fnd_data, 4);
                if(retval < 0) {
                    printf("write error!!\n");
                    return -1;
                }

                //when alarm time... 
                if( shm_addr[121] == 1 && 
                        fnd_data[0] == shm_addr[117] && 
                        fnd_data[1] == shm_addr[118] && 
                        fnd_data[2] == shm_addr[119] && 
                        fnd_data[3] == shm_addr[120] && 
                        shm_addr[116] == 0 ) 
                    {
                    check = 1;
                }

                //alarm function
                if( check == 1) {  //alarm on
                
                    if(shm_addr[127] == shm_addr[126]) {//when valid answer inserted

                        shm_addr[116] = 0;
                        shm_addr[121] = 0;
                        check = 0; 
                        buzzData = 2;

                        retval = write(dev_buzzer, &buzzData, 1); 

                        lednum = 0;
                        write(dev_led, &lednum, 1);

                        memset( ledtext, 0x00, sizeof(ledtext) );
                        strcpy( ledtext, "ByeBye" );

                    }
                    else {

                        if(shm_addr[44] % 2 == 0) {
                            buzzData = 1;
                            lednum = 240;
                            write(dev_dot_font, fpga_number_1[0], sizeof(fpga_number_1[0]));
                        }
                        else {
                            buzzData = 0;
                            lednum = 15;
                            write(dev_dot_font, fpga_number_1[1], sizeof(fpga_number_1[1]));
                        }
                        //write dot and lcd

                        retval = write(dev_led, &lednum, 1);
                        lednum=0;

                        shm_addr[122] = 1; //problem solving mode on..

                        //write problem to text lcd
                        memset( ledtext, 0x00, sizeof(ledtext) );
                        ledtext[0] = shm_addr[123] + 48;
                        ledtext[1] = shm_addr[125];
                        ledtext[2] = shm_addr[124] + 48;
                        ledtext[3] = 61;

                        if(shm_addr[127] != 1000)
                            ledtext[4] = shm_addr[127] + 48;

                    }
                    //write buzzer info
                    retval = write(dev_buzzer, &buzzData, 1);
                    if(retval < 0) {
                        printf("buz write error..\n");
                        return -1;
                    }

                    shm_addr[122] = 1; //problem solving mode on..
                    retval = write(dev_text_lcd, ledtext, MAX_STRING);

                    if(retval < 0) {
                        printf("TEXT_LCD write error..\n");
                        return -1;
                    }

                }
                else {
                    //when alarm off, init led and dot
                    write(dev_dot_font, fpga_number_1[2], sizeof(fpga_number_1[2]));
                    lednum = 0;
                    write(dev_led, &lednum, 1);
                }
                break;
            default:
                break;
        }
        if(exit_flag){
            break;
        } 
		
		usleep(200000);

    }

}
int main_process(int shm_id)
{
    unsigned char push_sw_buff[MAX_BUTTON];
    int exit_flag = 0;
    cursor curdot;
    int idxCount[9] = {0};
    time_t ctime;
    struct tm *tm;
    int dev_switch;
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
    
    int tmphour     = 0;
    int tmpmin      = 0;
    int cur10hour   = 0;
    int cur10min    = 0;
    int curmin      = 0;
    int curhour     = 0;

    int textMode    = 0;
    int countnum    = 0;
    int countType   = 0;
    int count1      = 0;
    int count10     = 0;
    int count100    = 0;

    int tmpcount2   = 0;
    int tmpcount4   = 0;
    int tmpcount8   = 0;
    int tmpcount10  = 0;

    int buttidx     = 0;
    int buttcount   = 0;
    int premode     = -1;
    int preval      = -1;

    int num1;
    int num2;
    int operator;
    int answer;
    int input;

   	int i; 

	int status;

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

        switch( shm_addr[0] ){


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

				textMode 	= 0;
				curStrnum  	= 0;
				pushcount 	= 0;
				pushcount2 	= 0;
				curdot.row 	= 6;
				curdot.col 	= 0;
				printf("ini");

                memset( idxCount, 0x00, sizeof(idxCount) );
        
                read(dev_switch, &push_sw_buff, buff_size);

                //set type of number
                if(push_sw_buff[0] == 1) {
                    countType++;
                    countType = countType % 4;
                }
                //set led number
                if(countType == 0) {   
                    countnum = 10;
                    shm_addr[6] = 64;
                }

                else if(countType == 1) { 
                    countnum = 8;
                    shm_addr[6] = 32;
                }

                else if(countType  == 2) { 
                    countnum = 4;
                    shm_addr[6] = 16;
                }

                else {  
                    countnum = 2;
                    shm_addr[6] = 128;
                }


                if(push_sw_buff[1] == 1) {  //100
                    tmpcount += (countnum*countnum);	
                }
                else if(push_sw_buff[2] == 1) { //10
                    tmpcount += countnum;
                }
                else if(push_sw_buff[3] == 1) {  //1
                    tmpcount += 1;
                }

                if(countnum == 8) {   //calcuate octal number

                   	tmpcount8 = changeToOcta(tmpcount);
                    tmpcount8 = tmpcount8 % 1000;
                    count100 = tmpcount8 / 100;
                    count10 = (tmpcount8 - count100 * 100)/10;
                    count1 = tmpcount8 - count100*100 - count10 * 10;
                }
                else if(countnum == 4) { //calculate four number

                    tmpcount4 = changeToFour(tmpcount);
                    tmpcount4 = tmpcount4 % 1000;
                    count100 = tmpcount4 / 100;
                    count10 = (tmpcount4 - count100 * 100)/10;
                    count1 = tmpcount4 - count100*100 - count10 * 10;
                }
                else if(countnum == 2) {//calculate binary number

                    tmpcount2 = changeToTwo(tmpcount);
                    tmpcount2 = tmpcount2 % 1000;
                    count100 = tmpcount2 / 100;
                    count10 = (tmpcount2 - count100 * 100)/10;
                    count1 = tmpcount2 - count100*100 - count10 * 10;

                }
                else {// calcuate decimal number

                    tmpcount10 = tmpcount % 1000;
                    count100 = tmpcount10 / 100;
                    count10 = (tmpcount10 - count100 * 100)/10;
                    count1 = tmpcount10 - count100*100 - count10 * 10;

                }

                //save calcuated number
                shm_addr[2] = count100;
                shm_addr[3] = count10;
                shm_addr[4] = count1;


				break;


            case 3:



                read(dev_switch, &push_sw_buff, buff_size);

                //initialize
                buttcount = 0;
                pushcount2 = 0;
                tmpcount = 0;
                curdot.row = 6;
                curdot.col = 0;
                countType = 0;

                for(i = 0 ; i < 9 ; i++) {
                    if(push_sw_buff[i] == 1)
                    buttcount++;
                }	

                if(buttcount > 2){
                    continue;
                }
                else if((push_sw_buff[4] * push_sw_buff[5]) == 1) {  //Mode change

                    premode = textMode;
                    textMode++;
                    textMode = textMode % 2;
                    shm_addr[7] = textMode;
                    //initialize
                    preval = -1;
                    memset( idxCount, 0x00, sizeof(idxCount) );
                    
                }
                else if((push_sw_buff[1] * push_sw_buff[2]) == 1) { //clear string

                    for(i = 8 ; i < 40 ; i++) 
                        shm_addr[i] = 0;	

                    curStrnum = 0;
                    shm_addr[40] = curStrnum;
                    pushcount = 0;
                    preval = -1;
                    printf("clear text....\n");
                }
                else if((push_sw_buff[7] * push_sw_buff[8]) == 1) { //insert space

                    makeString(shm_addr, curStrnum, ' ');
                    if(curStrnum != 32)
                    curStrnum++;
                    shm_addr[40] = curStrnum;
                    pushcount++;
                }
                else if(buttcount == 1) {	

                    for(i = 0 ; i < 9 ; i++) {
                        if(push_sw_buff[i] == 1) {
                            buttidx = i;
                            break;
                        }
                    }

                    if(textMode == 0) {  //alphabet mode

                    if(preval != -1 && preval == buttidx)
                        curStrnum--;

                    makeString(shm_addr, curStrnum, charTable[buttidx][idxCount[buttidx]]);
                    idxCount[buttidx]++;
                    idxCount[buttidx] = idxCount[buttidx] % 3;

                    preval = buttidx;


                    if(curStrnum != 32)
                        curStrnum++;

                    shm_addr[40] = curStrnum;
                    }
                    else if(textMode == 1) { //number mode

                        makeString(shm_addr, curStrnum, buttidx + 49);	

                        if(curStrnum != 32)
                        curStrnum++;
                        shm_addr[40] = curStrnum;
                    }
                    pushcount++;

                }
                // for fnd counter
                countPush(shm_addr, pushcount);


				break;


            case 4:


                read(dev_switch, &push_sw_buff, buff_size);

                memset( idxCount, 0x00, sizeof(idxCount) );
                curStrnum = 0;
                pushcount = 0;
                buttcount = 0;
                textMode = 0;
                preval = -1;
                premode = -1;


                shm_addr[44] = tm->tm_sec;
                ///move cursor...


                for(i = 0 ; i < 9 ; i++) {
                    if(push_sw_buff[i] == 1)
                    buttcount++;
                }

                if(buttcount > 1)  //valid for one button input
                    continue;
                else if(push_sw_buff[1] == 1) {  //go up
                    printf("cursor go up!!..\n");
                    if(curdot.col > 0)
                    	curdot.col--;
                }
                else if(push_sw_buff[3] == 1) {
                    printf("cursor go left!!..\n"); //go left
                    if(curdot.row < 6)
                    	curdot.row++;
                }
                else if(push_sw_buff[5] == 1) {
                    printf("cursor go right!!..\n");  //go right
                    if(curdot.row > 0)
                    	curdot.row--;
                }
                else if(push_sw_buff[7] == 1) {  //go down
                    printf("cursor go down!!..\n");
                    if(curdot.col < 9) 
                    	curdot.col++;
                }

                if(buttcount > 0)
                    pushcount2++;

                shm_addr[42] = curdot.row;
                shm_addr[43] = curdot.col;

                ///// coloring dot
                if(push_sw_buff[0] == 1) {   //reset all setting
                    refreshDot(shm_addr);
                    curdot.row = 6;
                    curdot.col = 0;
                    shm_addr[42] = curdot.row;
                    shm_addr[43] = curdot.col;
                    shm_addr[115] = 0;
                    pushcount2 = 0;
                }
                else if(push_sw_buff[2] == 1) { //cursor on / off
                    shm_addr[115] = 1 - shm_addr[115];
                }
                else if(push_sw_buff[4] == 1) {
                    shm_addr[makeIdx(curdot.row, curdot.col)] = 1;
                }
                else if(push_sw_buff[6] == 1) { //refresh Dot
                    refreshDot(shm_addr);	
                }
                else if(push_sw_buff[8] == 1) {
                    inverseDot(shm_addr);
                }
                //fnd counter
                countPush(shm_addr, pushcount2);

				break;
            case 5:
   
                curdot.row = 6;
                curdot.col = 0;
                buttcount = 0;
                pushcount2 = 0;
                addhour = 0;
                addmin = 0;
                shm_addr[44] = tm->tm_sec;					
                shm_addr[127]=1000;
                //for time count
                read(dev_switch, &push_sw_buff, buff_size);

                if(push_sw_buff[0] == 1 && shm_addr[116] == 0) {
                    shm_addr[116] = 1;
                    printf("alarm setting mode...\n");
                    addmin = 0;
                    addhour = 0;
                }
                else if(push_sw_buff[0] == 1 && shm_addr[116] == 1) {
                    shm_addr[116] = 0;
                    printf("alarm setting mode finished...\n");
                }


                if(shm_addr[116] == 1) {  //alarm setting mode
                    ////for led

                    ///for switch

                    //calculate current time
                    if(push_sw_buff[1] == 1) {
                        addmin = 0;
                        add10min = 0;
                        addhour = 0;
                        add10hour = 0;
                    }
                    else if(push_sw_buff[3] == 1) {
                        tmphour = (tmphour + 1) % 24;
                    }
                    else if(push_sw_buff[4] == 1) {
                        tmpmin = (tmpmin + 1) % 60;
                    }

                    add10hour = (tmphour/10);
                    addhour = (tmphour%10);
                    add10min = (tmpmin/10);
                    addmin = (tmpmin%10);

                    shm_addr[1] = add10hour;
                    shm_addr[2] = addhour;
                    shm_addr[3] = add10min;
                    shm_addr[4] = addmin;


                    //set time for alarm
                    if(push_sw_buff[2] == 1 && shm_addr[121] == 0) {
                        printf("alarm saved... %d%d:%d%d\n",add10hour, addhour, add10min,addmin);
                        shm_addr[117] = add10hour;
                        shm_addr[118] = addhour;
                        shm_addr[119] = add10min;
                        shm_addr[120] = addmin;
                        //alarm enable
                        shm_addr[121] = 1;
                    }
                    //when alarm disabled
                    else if(push_sw_buff[2] == 1 && shm_addr[121] == 1) {
                        printf("alarm disabled..\n");
                        shm_addr[121] = 0;
                    }
                }
                else {   //print current time	
                    tmphour = tm->tm_hour;
                    tmpmin = tm->tm_min;

                    cur10hour = tmphour/10;
                    curhour = tmphour%10;
                    cur10min = tmpmin/10;
                    curmin = tmpmin%10;

                    shm_addr[1] = cur10hour;
                    shm_addr[2] = curhour;  
                    shm_addr[3] = cur10min;
                    shm_addr[4] = curmin;

                    if(shm_addr[122] == 1) {  //problem solving mode 

                        if(num1 == 0  && num2 == 0) {
                            //make problem one time

                            makeNumber(&num1, &num2);
                            operator = makeOperator(num1, num2);
                            answer = makeAnswer(num1, num2, operator);
                        }
                        //save problem to print text lcd
                        shm_addr[123] = num1;
                        shm_addr[124] = num2;
                        shm_addr[125] = operator;
                        shm_addr[126] = answer;

                        buttcount = 0;
                        for(i = 0 ; i < 9 ; i++) {
                            if(push_sw_buff[i] == 1)
                            buttcount++;
                        }

                        if(buttcount == 1){
                            for(i = 0 ; i < 9 ; i++) {
                            if(push_sw_buff[i] == 1)
                                input = i + 1;
                            }
                            shm_addr[127] = input;
                        }

                    }
                    else {
                        num1 = 0;
                        num2 = 0;
                    }

                }

				break;
            default:
				break;
        }

		if( exit_flag ){
			break;
		}
		usleep(200000);

    }


	wait(&status);
	wait(&status);

    close(dev_switch);

	shmdt(shm_addr);
	shmctl(shm_id, IPC_RMID, (struct shmid_ds *)NULL);

	return 0;


}

void makeString(int* shm_addr, int curStrnum, char val) {  //write string to shared memory
  int i;
  if(curStrnum + 1 > 32) {
    for(i = 8 ; i < 39 ; i++) {
      shm_addr[i] = shm_addr[i + 1];
    }
    shm_addr[39] = val;
  }		
  else {
    shm_addr[curStrnum+8] = val;
  }
}

void writeString(int *shm_addr, unsigned char* str) { 	// writeString.
	int i;
	memset(str, 0x00, MAX_STRING);

	for( i = 0; i <shm_addr[40]; i++){
		str[i] = shm_addr[8+i];
	}
}

void countPush(int *shm_addr, int pushcount){
	pushcount %= 1000;
	shm_addr[1] = pushcount/1000;
	shm_addr[2] = (pushcount%1000)/100;
	shm_addr[3] = (pushcount%100)/10;
	shm_addr[4] = pushcount%10;
}

int changeToOcta(int num)
{
    if( num < 8 ){
        return num;
    }  
	return changeToOcta(num/8)*10 + num%8;
}

int changeToFour(int num)
{
    if( num < 4 ){
        return num;
    }   
	return changeToFour(num/4)*10 + num%4;

}

int changeToTwo(int num)
{
    if( num < 2 ){
        return num;
    }   
	return changeToTwo(num/2)*10 + num%2;

}

void initDotshm(int* shm_addr) //initialize dot data
{   
  int i;
  shm_addr[42] = 6;
  shm_addr[43] = 0;
  for(i = 45 ; i < 115 ; i++) {
    shm_addr[i] =0;
  }
}


void makeDotAry(int* shm_addr, char fpga[10]) {    //make dot data by using shared memory data
  int i, j;
  int sum;
  for(i = 0 ; i < 10 ; i++) {
    sum = 0;
    for(j = 45 + i ; j < 106 + i ; j = j + 10) {
      sum = sum + shm_addr[j] * ppow(2, (j - 45 - i)/10);
    }
    fpga[i] += sum;
  }
}


void refreshDot(int* shm_addr) {  //refresh dot array
  int i, j;
  for(i = 0 ; i < 10 ; i++) {
    for(j = 45 + i ; j < 106 + i ; j = j + 10) {
      shm_addr[j] = 0;
    }
  }

}

void inverseDot(int* shm_addr) {   //inverse dot data
  int i, j;
  for(i = 0 ; i < 10 ; i++) {
    for(j = 45 + i ; j < 106 + i ; j = j + 10) {
      shm_addr[j] = 1 - shm_addr[j];
    }
  }
}


int ppow(int num, int mul) {  //same as pow in math.h
  int i;
  int ret = 1;
  for(i = 0 ; i < mul ; i++) {
    ret = ret*num;
  }
  return ret;
}

int makeIdx(int row, int col){		// make Index
	return row * 10 + col + 45;
}


void makeNumber(int *num1, int *num2){ 		// make number

    srand((int)time(NULL));
    *num1 = rand() % 4 + 1;
    *num2 = rand() % 4 + 1;
}

int makeOperator(int num1, int num2){		// make operator

    int randNum; 
    srand((int)time(NULL));
    randNum = rand()%2;

    if( randNum == 0 ){
        return 43;      // +
    }
    else{
		if( num1 * num2 > 10 )
			return 43;		// +
		else
        	return 42;      // *
    }
    
}

int makeAnswer(int num1, int num2, int operator) {  //make answer for alarm clock
  if(operator == 43)
    return num1+num2;
  else(operator == 42);
    return num1 * num2;
}
