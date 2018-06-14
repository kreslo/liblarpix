//CLOCKx2 generator
#define CLOCKx2_DIVIDER 0x43c00000

#define SYSTEM_RESET 0x43c00004

// UART CHANNEL A
#define UART54_A_SEND 0x43c10008 
#define UART54_A_RECV 0x43c10000 
//#define UART54_A_IRQR 0x43c10000 

// UART CHANNEL B
#define UART54_B_SEND 0x43c20008 
#define UART54_B_RECV 0x43c20000 
//#define UART54_B_IRQR 0x43c20000 

//RGB LEDS
#define LED1_B  0x43c30000
#define LED1_G  0x43c30004
#define LED1_R  0x43c30008
#define LED2_B  0x43c3000c
#define LED2_G  0x43c30010
#define LED2_R  0x43c30014

//ZMQ data backend
#define EVLEN 8

int setCLKx2(int FkHz); // set PIXLAR CLOCKx2 output frequency, kHz
int rgb(int r1, int g1, int b1, int r2, int g2, int b2); //values are given in percents 0-100
int uart54_send(int chan, uint64_t *buf, int num); // send 54-bits word to channel chan (0->A, 1->B)
int uart54_recv(int chan, uint64_t *buf, int num); // blocks until receive requested num words
int uart54_available(int chan); //returns 1 if word is available in buffer, 0 otherwise
int system_reset(); //issues system reset pulse for UART and PIXLAR asics


