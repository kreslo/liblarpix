
#include <zmq.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/timeb.h>
#include "pixlar.c"
#include <time.h>

void *context = NULL;

//  Socket to send data to clients
void *publisher = NULL;

struct timeb mstime0, mstime1;

uint8_t evbuf[EVLEN];

volatile int bufbusy=0;


void dump(volatile unsigned char * ptr)
{
   int i;
     for (i = 0; i < 8; ++i)
        printf("%02x", *(unsigned char*)(ptr+7-i));
        printf("\n");
 
}

void transfer_complete (void *data, void *hint) //call back from ZMQ sent function, hint points to subbufer index
{
bufbusy=0;
// printf("Subbuf transmission complete.\n");
}


void sendout(void * evbuf)
{
zmq_msg_t msg;
zmq_msg_init_data (&msg, evbuf, EVLEN , transfer_complete, NULL);
//printf("Scheduling subbuf %d for sending..\n",sbitosend);
zmq_msg_send (&msg, publisher, ZMQ_DONTWAIT);
//printf("done..\n");
zmq_msg_close (&msg);
}

void printdate()
{
    char str[64];
    time_t result=time(NULL);
    sprintf(str,"%s", asctime(gmtime(&result))); 
    str[strlen(str)-1]=0; //remove CR simbol
    printf("%s ", str); 
}




int main (int argc, char **argv)
{

int rv;
context = zmq_ctx_new();

//  Socket to send data to clients
publisher = zmq_socket (context, ZMQ_PUB);
rv = zmq_bind (publisher, "tcp://*:5556");
if(rv<0) {printdate(); printf("Can't bind tcp socket for data! ERRNO=%d. Exiting.\n",errno); return 0;}
printdate(); printf ("pixlar_server: data publisher at tcp://5556\n");


    off_t offsetA = UART54_A_RECV;
    off_t offsetB = UART54_B_RECV;
    size_t len = 8;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_baseA = (offsetA / pagesize) * pagesize;
    off_t page_offsetA = offsetA - page_baseA;
    off_t page_baseB = (offsetB / pagesize) * pagesize;
    off_t page_offsetB = offsetB - page_baseB;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
//    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    volatile unsigned char *memA = mmap(NULL, page_offsetA + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_baseA);
    if (memA == MAP_FAILED) {
        perror("Can't map A memory");
        return -1;
    }
    volatile unsigned char *memB = mmap(NULL, page_offsetB + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_baseB);
    if (memB == MAP_FAILED) {
        perror("Can't map B memory");
        return -1;
    }



while(1) //main loop
{

    if(memA[page_offsetA+len-1]>=0x80 && bufbusy==0)
    {
    printf("A:");
    dump(memA+page_offsetA);
    memcpy(evbuf,(void*)(memA+page_offsetA),8);
    bufbusy=1;
    sendout(evbuf);
    memA[page_offsetA+len-1]=0;
    } 
    if(memB[page_offsetB+len-1]>=0x80 && bufbusy==0)
    {
    printf("B:");
    dump(memB+page_offsetB);
    memcpy(evbuf,(void*)(memB+page_offsetB),8);
    bufbusy=1;
    sendout(evbuf);
    memB[page_offsetB+len-1]=0;
    } 



}


return 0;
}



