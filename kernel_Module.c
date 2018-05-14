/*
 * lab4p1_kernel.c
 *
 *  Created on: Oct 6, 2016
 *      Author: Zhentao Xie
 */

#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <linux/time.h>
#include <rtai.h>
#include <rtai_sem.h>



MODULE_LICENSE("GPL");

static RT_TASK mytask;
RTIME Basep;




static void rt_process(int t)
{
	unsigned long *PBDR,*PBDDR;  //create pointer to port B DR/DDR
	unsigned long *ptr;
	ptr = (unsigned long*)__ioremap(0x80840000,4096,0);
	PBDR = ptr+1;
	PBDDR = ptr+5;
	*PBDDR = 0xE0;  // give input, pbddr 11100000
	*PBDR |= 0x20; //00100000 Turn on red PortB5 to indicate kernel is install
	struct timeval t;
	unsigned long timestamp;

	while(1)
	{
		if((*PBDR & 0x01)==0)
		{
			do_gettimeofday(&t);
			timestamp = 1000000*t.tv_sec+t.tv_usec;
			rtf_put(1,&timestamp,sizeof(timestamp));
			printk("\n Sent timestamp:%lu",timestamp);
			*PBDR &= 0x7F; //01111111 Turn off green PortB7
			*PBDR |= 0x40; //01000000 Turn on yellow PortB6 to indicate timestamp is sent
		}
		else
		{
			*PBDR &= 0xBF; //10111111 Trun off yellow PortB6
			*PBDR |= 0x80; //10000000 Turn on green PortB7
		}
          rt_task_wait_period();
	}
}

int init_module(void)
{
	rt_set_periodic_mode(); //set to periodic to mode
	Basep = start_rt_timer(nano2count(1000000)); //1 milli-sec
	rt_task_init(&mytask,rt_process,0,256,0,0,0);
	rt_task_make_periodic(&mytask,rt_get_time()+0*Basep,75*Basep);
	rtf_create(1,1*sizeof(unsigned long)); //create fifo
	return 0;
}


void cleanup_module(void)
{
	rt_task_delete(&mytask); //delete real time task
	stop_rt_timer();         //stop timer
	rtf_destroy(0);          //destroy fifo 0
	unsigned long *PBDR,*PBDDR;  //create pointer to port B DR/DDR
	unsigned long *ptr;
	ptr = (unsigned long*)__ioremap(0x80840000,4096,0);
	PBDR = ptr+1;
	PBDDR = ptr+5;
	*PBDDR = 0xE0;  // give LEDs output, pbddr 11100000
	*PBDR &= 0x1F; //Turn off red yellow green LEDs
}
