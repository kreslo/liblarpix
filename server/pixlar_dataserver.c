
#include <zmq.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <termios.h>
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

#include <sys/select.h>
#include <termios.h>

struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}


int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

void *context = NULL;

//  Socket to send data to clients
void *publisher = NULL;

struct timeb mstime0, mstime1;

uint8_t evbuf[EVLEN];

volatile int bufbusy=0;

/*
int getch (void)
{
        int ch;
        struct termios oldt, newt;

        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON|ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        return ch;
}
*/

void transfer_complete (void *data, void *hint) //call back from ZMQ sent function, hint points to subbufer index
{
bufbusy=0;
// printf("Subbuf transmission complete.\n");
}


void sendout(void * evbuf, int bytes)
{
zmq_msg_t msg;
zmq_msg_init_data (&msg, evbuf, bytes , transfer_complete, NULL);
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

    set_conio_terminal_mode();

int rv;
uint64_t buf[1024];

uint64_t w64;
context = zmq_ctx_new();
long long unsigned rec_wA=0, rec_wB=0, rec_wC=0, rec_wD=0;
//  Socket to send data to clients
publisher = zmq_socket (context, ZMQ_PUB);
rv = zmq_bind (publisher, "tcp://*:5556");
if(rv<0) {printdate(); printf("Can't bind tcp socket for data! ERRNO=%d. Exiting.\n",errno); return 0;}
printdate(); printf ("pixlar_server: data publisher at tcp://5556\n");
bufbusy=0;


    int fd[4];
    fd[0] = open("/dev/uart640", O_RDWR); printf("open /dev/uart640 fd=%d\n",fd[0]);
    fd[1] = open("/dev/uart641", O_RDWR); printf("open /dev/uart641 fd=%d\n",fd[1]);
    fd[2] = open("/dev/uart642", O_RDWR); printf("open /dev/uart642 fd=%d\n",fd[2]);
    fd[3] = open("/dev/uart643", O_RDWR); printf("open /dev/uart643 fd=%d\n",fd[3]);

int recvd=0;

while(1) //main loop
{
    if(bufbusy==0){    recvd=read(fd[0],buf,1024*8);    rec_wA=rec_wA+recvd/8; if(recvd>0) {   bufbusy=1;   sendout(buf,recvd);}}
    if(bufbusy==0){    recvd=read(fd[1],buf,1024*8);    rec_wB=rec_wB+recvd/8; if(recvd>0) {   bufbusy=1;   sendout(buf,recvd);}}
    if(bufbusy==0){    recvd=read(fd[2],buf,1024*8);    rec_wC=rec_wC+recvd/8; if(recvd>0) {   bufbusy=1;   sendout(buf,recvd);}}
    if(bufbusy==0){    recvd=read(fd[3],buf,1024*8);    rec_wD=rec_wD+recvd/8; if(recvd>0) {   bufbusy=1;   sendout(buf,recvd);}}
     
    if(kbhit()) {if(getch()==3) return 0; printf("Received words:  A:%lld B:%lld C:%lld D:%lld \n",rec_wA, rec_wB,rec_wC, rec_wD);rec_wA=0; rec_wB=0; rec_wC=0; rec_wD=0; }
//    if(recvd>0) printf("Received words:  A:%lld B:%lld C:%lld D:%lld \n",rec_wA, rec_wB,rec_wC, rec_wD);


}

close(fd[0]);
close(fd[1]);
close(fd[2]);
close(fd[3]);
return 0;
}



