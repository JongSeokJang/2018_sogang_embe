#include <jni.h>
#include "android/log.h"
#include <unistd.h>
#include <fcntl.h>


/*
 * Class:     com_example_androidex_MainActivity2
 * Method:    myfunc_open
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_myfunc_1open
  (JNIEnv *env, jobject obj)
{
	int fd = open("/dev/dev_driver", O_RDWR);
	return fd;
}

/*
 * Class:     com_example_androidex_MainActivity2
 * Method:    myfunc_write
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_myfunc_1write
  (JNIEnv *env, jobject obj, jint fd, jint flag)
{

	char buf[200];
	int result;

	if( flag == 100 ){
		result = write(fd,buf,100);
	}
	else{
		result = write(fd,buf,200);
	}
	return result;
}

/*
 * Class:     com_example_androidex_MainActivity2
 * Method:    myfunc_close
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_myfunc_1close
  (JNIEnv *env, jobject obj, jint fd)
{
	return close(fd);
}
