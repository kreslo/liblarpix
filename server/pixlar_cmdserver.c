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
#include "pixlar.h"
#include <time.h>

void *context = NULL;

//  Socket to respond to clients
void *responder = NULL;
struct timeb mstime0, mstime1;
uint64_t  nstarts=0, nstops=0;
char dummy[256];

void printdate()
{
    char str[64];
    time_t result=time(NULL);
    sprintf(str,"%s", asctime(gmtime(&result))); 
    str[strlen(str)-1]=0; //remove CR simbol
    printf("%s ", str); 
}

int SetFreq(int freq)
{
  setCLKx2(freq);
  return 1;
}


int SendWord(uint64_t wd)
{
  printf("ASIC: sending to all channels ");
    dumpc(wd);
  uart54_send(0, &wd, 1);
  uart54_send(1, &wd, 1);
  uart54_send(2, &wd, 1);
  uart54_send(3, &wd, 1);
  return 1;
}


int main (int argc, char **argv)
{

int rv;
context = zmq_ctx_new();

//  Socket to respond to clients
responder = zmq_socket (context, ZMQ_REP);
rv=zmq_bind (responder, "tcp://*:5555");
if(rv<0) {printdate(); printf("Can't bind tcp socket for command! Exiting.\n"); return 0;}
printdate(); printf ("pixlar_server: listening at tcp://5555\n");


zmq_msg_t request;

char cmd[32]; //command string
uint64_t arg=0;

while (1) {  // main loop

//Check next request from client
zmq_msg_init (&request);
//if(zmq_msg_recv (&request, responder, ZMQ_DONTWAIT)==-1) {zmq_msg_close (&request);  polldata(); senddata(); sendstats(); sendstats2(); usleep(polldelay*1000); continue;} 
if(zmq_msg_recv (&request, responder, ZMQ_DONTWAIT)==-1) continue; 
memcpy(cmd,(char*)zmq_msg_data(&request),7); cmd[7]=0;
arg=strtoull((char*)(zmq_msg_data(&request)+8), NULL, 0);
printdate(); printf ("Received Command %s 0x%llx (decimal %lld) \n",cmd, arg, arg );
rv=0;
 if(strcmp(cmd, "SETFREQ")==0) rv=SetFreq((int)arg); //get 8-th byte of message - start of argument
 else if (strcmp(cmd, "SNDWORD")==0) rv=SendWord(arg);
 else if (strcmp(cmd, "SYRESET")==0) rv=system_reset();
 else if (strcmp(cmd, "GETSTAT")==0) rv=uart54_getstats(arg, &nstarts, &nstops);
 else if (strcmp(cmd, "SETCONF")==0) ;//rv=configu(*(uint8_t*)(zmq_msg_data(&request)+8), (uint8_t*)(zmq_msg_data(&request)+9), zmq_msg_size (&request)-9); 
 else if (strcmp(cmd, "GET_SCR")==0) ;//rv=getSCR(*(uint8_t*)(zmq_msg_data(&request)+8),buf); 

//  Send reply back to client
zmq_msg_t reply;

if (strcmp(cmd, "GETSTAT")==0)
{
  sprintf(dummy, "%lld %lld",nstarts,nstops);
  zmq_msg_init_size (&reply, strlen(dummy)+1);
  sprintf(zmq_msg_data (&reply), "%s",dummy);
}
else
{
 zmq_msg_init_size (&reply, 5);
 if(rv>0) sprintf(zmq_msg_data (&reply), "OK");
 else sprintf(zmq_msg_data (&reply), "ERR");
} 
printf("Sending reply %s\n",(char*)zmq_msg_data (&reply));
zmq_msg_send (&reply, responder, 0);
zmq_msg_close (&reply);

} //end main loop


return 0;
}


