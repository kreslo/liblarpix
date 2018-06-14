/// sends literal command without parameters to feb
#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


void usage()
{
 printf("Usage: ");
 printf("pixlar_ctl <socket> <CMD> <ARG>\n");
 printf("Socket string, example: tcp://localhost:5555 \n");

}



int main (int argc, char **argv)
{
int rv=0;
if(argc<3 || argc>4) { usage(); return 0;}
char cmd[32];
if(argc>3) sprintf(cmd,"%s %s", argv[2],argv[3]);
else sprintf(cmd,"%s", argv[2]);
//if(argc==4) mac5=atoi(argv[3]); else mac5=255;
void * context = zmq_ctx_new ();
//  Socket to talk to server
printf ("Connecting to driver...\n");
void *requester = zmq_socket (context, ZMQ_REQ);
rv=zmq_connect (requester, argv[1]); if(rv<0) {printf("Connection to %s failed!\n",argv[1]); return 0;}
//zmq_connect (requester, "ipc://command");
int request_nbr;
zmq_msg_t request;
zmq_msg_init_size (&request, strlen(cmd));
memcpy(zmq_msg_data (&request), cmd,strlen(cmd));
printf ("Sending command %s...", cmd);
zmq_msg_send (&request, requester, 0);
zmq_msg_close (&request);
zmq_msg_t reply;
zmq_msg_init (&reply);
zmq_msg_recv (&reply, requester, 0);
printf ("Received reply: %s\n", (char*)zmq_msg_data (&reply));
zmq_msg_close (&reply);
zmq_close (requester);
zmq_ctx_destroy (context);
return 0;
}

