#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

struct my_data{
	int a,b,c;
};

struct my_new_function_data{
	int length;
	int data[10];
}

void mynew_function(struct my_new_function_data *numInfo, int num){

	int ii;
	int length;
	
	if( num < 10 ) 
		length = 1;
	else if ( num < 100 ) 
		length = 2;
	else if ( num < 1000) 
		length = 3;
	else if ( num < 10000 ) 
		length =4;

	
	for( ii = 0; ii < length; ii++){
		numInfo->data[ii] = num/10;
		num %= 10;
	}

	dataInfo.length = length;
	numInfo->length = length;

	
}


long newcall(struct my_data *data){
	int a, b;

	a=data->a;
	b=data->b;

	data->a=a+b;
	data->b=a-b;
	data->c=a%b;

	return 313;
}

int funcdev_init( void ) {
	return 0;
}
void funcdev_exit( void ) { }

EXPORT_SYMBOL( newcall );

module_init ( funcdev_init );
module_exit ( funcdev_exit );
MODULE_LICENSE("Dual BSD/GPL");
