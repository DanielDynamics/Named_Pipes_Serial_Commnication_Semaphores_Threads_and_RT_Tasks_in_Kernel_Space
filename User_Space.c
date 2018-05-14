/*
 *
 *
 *  Created on: Oct 6, 2016
 *      Author: Zhentao Xie
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "serial_ece4220.h"
#include <pthread.h>
#include <sys/time.h>
#include <rtai.h>
#include <rtai_lxrt.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>


//RTIME Basep;
sem_t semID;
int print_pipe_out;
struct BUFFER
{
   unsigned int x_GPS;
   unsigned long t_GPS;
}buf;


struct BUFFER_SEND
{
  unsigned int x0_s;
  unsigned long t0_s;
  unsigned long t_event_s;
  int thrd_num_s;
}bufsend;

void *Thrd_func_Kernel(void *ptr); //read kernel thread function
void *Thrd_func_Serial(void *ptr); //read serial thread function
void *Thrd_func_Print(void *ptr); // print data thread function
void *GrndThrd_func(void *ptr);



int main(void)
{
	sem_init(&semID,0,1);
	pthread_t thrd_K, thrd_S, thrd_P;

	pthread_create(&thrd_S, NULL, Thrd_func_Serial, NULL); // read serial
	pthread_create(&thrd_K, NULL, Thrd_func_Kernel, NULL); // read kernel
	pthread_create(&thrd_P, NULL, Thrd_func_Print, NULL);  // print data

	pthread_join (thrd_S,NULL);
	pthread_join (thrd_K,NULL);
	pthread_join (thrd_P,NULL);
	return 0;
}



void *Thrd_func_Kernel(void *ptr)
{
	printf("In reading kernel function\n");
	pthread_t GrndThrd[4];
	unsigned long timestamp;
	int kernel_in;
	if((kernel_in = open("/dev/rtf/1",O_RDONLY))<0)  //open a fifo 1 to read kernel
	{
		printf("pipe 1 open error\n");
		exit(-1);
	}
	else
	{
		printf("pipe 1 open successfully\n");
	}

	int dummy;
	dummy=system("mkfifo UtoP");
    printf("dummy is %d ,Named pipe UtoP opening...\n",dummy);
	if((print_pipe_out = open("UtoP",O_WRONLY))<0)  //open a named pipe UsrtoPrint to write to a thread to print
	{
		printf("pipe UtoP write open error\n");
		exit(-1);
	}
	else
	{
		printf("pipe UtoP  write open successfully\n");
	}

	int thread_num = 0;

	printf("Block to read timestamp from kernel\n");
	while(1)
	{
		//printf("Block to read timestamp from kernel\n");
		if((read(kernel_in, &timestamp, sizeof(timestamp)))<0) //read from kernel
		{
			printf("pipe 1 read error\n");
			exit(-1);
		}

		bufsend.x0_s = buf.x_GPS;
		//printf("buf.x_GPS is %u\n",buf.x_GPS);
		bufsend.t0_s = buf.t_GPS;
		//printf("buf.t_GPS is %lu\n",buf.t_GPS);
		bufsend.t_event_s = timestamp;
		bufsend.thrd_num_s = thread_num;
		//printf("thread number is:%d\n",thread_num);
		pthread_create(&GrndThrd[thread_num],NULL,GrndThrd_func,(void *)&bufsend);
		//pthread_join(GrndThrd[thread_num],NULL);  //can't add pthread_join at here
		thread_num++;
		if(thread_num>3)
		{
			thread_num=0;
		}


	}

}

void *GrndThrd_func(void *ptr)
{

	struct BUFFER_PRINT
	{
		unsigned int x0;
		unsigned int x1;
		float x_event;
		unsigned long t0;
		unsigned long t1;
		unsigned long t_event;
		int thrd_num_p;
	}bufprint;


	//sem_init(&semID,0,1);
	struct BUFFER_SEND *received_data = (struct BUFFER_SEND *)ptr;
	unsigned int x0_GPS, x1_GPS, temp_x;
	float x_st;
	unsigned long t0_GPS, t1_GPS, event_time;
	int num;
	x0_GPS = received_data->x0_s;
	t0_GPS = received_data->t0_s;
	event_time = received_data->t_event_s;
	num = received_data->thrd_num_s;
	temp_x = received_data->x0_s;
    //printf("x0_GPS :%u, t0_GPS :%lu, thrd_num:%d\n",x0_GPS,t0_GPS,num);
	while(temp_x == buf.x_GPS) {}

	sem_wait(&semID);
	x1_GPS = buf.x_GPS;
	t1_GPS = buf.t_GPS;
	x_st = ((float)x1_GPS-(float)x0_GPS)*((float)event_time-(float)t0_GPS)/((float)t1_GPS-(float)t0_GPS)+(float)x0_GPS;
	//sem_wait(&semID);

	bufprint.x0 = x0_GPS;
	bufprint.t0 = t0_GPS;
	bufprint.x_event = x_st;
	bufprint.t_event = event_time;
	bufprint.x1 = x1_GPS;
	bufprint.t1 = t1_GPS;
    bufprint.thrd_num_p = num;

	if(write(print_pipe_out,&bufprint,sizeof(bufprint))!=sizeof(bufprint))
	{
		printf("Named pipe UtoP write error\n");
		exit(-1);
	}

	//printf("x_0 :%d, t_0 :%lu, x_e :%.2f, t_e :%lu, x_1 :%d, t_1:%lu\n",x0_GPS,t0_GPS,x_st,event_time,x1_GPS,t1_GPS);
	//sem_post(&semID);
    pthread_exit(0);
}



void *Thrd_func_Serial(void *ptr)
{
	struct timeval time_GPS;
	int prt_id = serial_open(0,5,5);
	unsigned char x;
	ssize_t num_bytes;
	buf.x_GPS = 0;
	buf.t_GPS = 0;
	usleep(10);
	while(1)
	{
		//printf("Wait to read from serial\n");
		if(num_bytes=(read(prt_id,&x,1))<0)  //read from serial port
		{
			printf("Read serial error\n");
			exit(-1);
		}
		gettimeofday(&time_GPS,NULL);
		buf.x_GPS = (unsigned int)x;
		buf.t_GPS= 1000000*time_GPS.tv_sec+time_GPS.tv_usec;

		//printf("Data in buffer is: [%d,%lu]\n",buf.x_GPS,buf.t_GPS);
		fflush(stdout);
	}
}


void *Thrd_func_Print(void *ptr)
{
	struct BUFFER_PRINT_RECEIVE
	{
		unsigned int x0_p;
		unsigned int x1_p;
		float x_event_p;
		unsigned long t0_p;
		unsigned long t1_p;
		unsigned long t_event_p;
		int thrd_num_p_p;
	}bufprintreceive;


	int print_pipe_in;
	if((print_pipe_in = open("UtoP",O_RDONLY))<0)  //open a named pipe 1 to read kernel
	{
		printf("Named pipe UtoP for print read open error\n");
		exit(-1);
	}
	while(1)
	{
		if((read(print_pipe_in, &bufprintreceive, sizeof(bufprintreceive)))<0) //read from pipe 2 bufferprint
		{
			printf("Named pipe UtoP read error\n");
			exit(-1);
		}
		printf("Thread num: %d, x_0 :%d, t_0 :%lu, x_e :%.2f, t_e :%lu, x_1 :%d, t_1:%lu\n",bufprintreceive.thrd_num_p_p,bufprintreceive.x0_p,bufprintreceive.t0_p,bufprintreceive.x_event_p,bufprintreceive.t_event_p,bufprintreceive.x1_p,bufprintreceive.t1_p);
		sem_post(&semID);
	}

}












