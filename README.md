# Named_Pipes_Serial_Commnication_Semaphores_Threads_and_RT_Tasks_in_Kernel_Space
Scenario:
This project simulates the situation when we want to find the exact location of a real time event. Assume there is a camera mounted on a helicopter, which takes snapshots at different times. A GPS module provides the location data every 250ms. In order to find the exact location of each snapshot, we need to interpolate the two GPS data, one before the events happened and one after.

Lab Procedure:
In this project, a user space program needs to be created that consists of a main loop that receives data from serial port, collects a time-stamp and saves both pieces of information in a buffer. Two other pthreads should also be created. The job of the first pthread is to read data (the time-stamp of the real time event) from a FIFO, and to store the GPS data (provided by the main loop) before and after the real time event. The job of the second pthread is to receive the data (two GPS data and their time-stamps, and the time-stamp of the real time event) through a named pipe and print it out on the screen. In addition to the user space program, a kernel module will need to be installed. It will create a real time task, check the real time event, collect the time- stamp and send it through a FIFO. The overall flow of the program is as follows:

1. The main program will loop, receiving serial data and collecting time-stamps.
2. When the data is received, it is saved in a common memory buffer by the main program.
3. The real-time task waits for a push button (B0) to be pressed, then passes the time-stamp to the first pthread through a FIFO.
4. The first pthread reads this time-stamp from the FIFO, collects the GPS data and time-stamps right before and after the real time event, and sends these data through a named pipe to the second pthread.
5. The second pthread then prints out all the data and time-stamps.
This project is broken down into four parts. 

Part 1: Main Program and Serial Communication
In this part,run a program to receive data from the serial port and make sure the serial communication is working properly. Modify the source code to get the time-stamp of the data received. The data and time-stamps should be placed in a common buffer. A pthread should be created, which will have access to the buffer. Print the data to the screen, just to make sure that you are getting them correctly.
An executable file is provided, which sends the GPS data (8 bit integer values (char)) every 250 ms through serial port with baud rate of 115200, 8 bit, no parity and 1 stop bit (this will be referred to as the GPS event). A function to receive the data from the serial port on COM2 (/dev/ttyAM1) of the embedded device will be given to you. This function will be a part of the main program.

Part 2: Kernel Module
The first purpose of the module is to set up a FIFO that will be used for communication between the user space program and a real time task. The second purpose of the module is to create the real time task that writes to the FIFO. The real time task should go off every 75ms. Remember to check for errors when setting up the task and FIFOs.
The real time task should check the push button on port B0, collect a time-stamp when it is pressed (this will be referred to as the real time or push button event) and write the time-stamp to the FIFO.
Note 1: make sure that the rtai_fifos.o and rtai_ksched.o modules are installed (use lsmod). If they are not, you need to locate and install them (using insmod).
Note 2: the pin, which is connected to the first push button (B0) should be configured as an input in order to check when it is pressed. Note 3: it might need to do the de-bounce operation when the push button is pressed. Instead of usleep command (which is not available in kernel module), you can use rt_sleep() command.

Part 3: Main Pthread and Dynamic Pthreads
In this part, it needs to modify the pthread that you created in part 1. The purpose of this main pthread is to receive data from the kernel module through the FIFO and to read the last GPS data and time-stamp from the common buffer, which is being updated by the main program. In part 4 we will do an interpolation of the GPS data corresponding to the push button event. Therefore, here we need to wait for the next GPS event and save the corresponding data and time-stamp. The final data consists of the real time event’s time-stamp, the previous and the next GPS data and their corresponding time-stamps.

Part 4: Interpolation and Display
A printing thread should be created to display the final output for each push button event. The final data (the real time event’s time-stamp, the previous and the next GPS data and their corresponding time-stamps) are to be received by this printing thread through a named pipe. Using the final data, you will interpolate the GPS value corresponding to the push button event. Finally, this thread will print: the time-stamp and value of the GPS event prior to the push button event, the time-stamp and the (estimated) GPS value of the push button event, and the time- stamp and value of the GPS event after the push button event.
NOTE 1: Before running the user space program, make sure that the kernel module has been installed.
NOTE 2: In Figure 1, a basic flowchart of the program is shown. It might need more pthreads, buffers, semaphores, etc. in order to complete the assignment
