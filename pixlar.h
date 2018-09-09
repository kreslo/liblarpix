//CLOCKx2 generator
#define CLOCKx2_DIVIDER 0x43c00000

#define SYSTEM_RESET 0x43c00004
// defines below are not used any more. Communication is made via uart64 driver.
// UART CHANNEL A
#define UART54_A_STOPS 0x43c01018 
#define UART54_A_STARTS 0x43c01010 
#define UART54_A_SEND 0x43c01008 
#define UART54_A_RECV 0x43c01000 
//#define UART54_A_IRQR 0x43c10000 

// UART CHANNEL B
#define UART54_B_STOPS 0x43c02018 
#define UART54_B_STARTS 0x43c02010 
#define UART54_B_SEND 0x43c02008 
#define UART54_B_RECV 0x43c02000 
//#define UART54_B_IRQR 0x43c20000 


// UART CHANNEL C
#define UART54_C_STOPS 0x43c03018 
#define UART54_C_STARTS 0x43c03010 
#define UART54_C_SEND 0x43c03008 
#define UART54_C_RECV 0x43c03000 
//#define UART54_C_IRQR 0x43c40000 


// UART CHANNEL D
#define UART54_D_STOPS 0x43c04018 
#define UART54_D_STARTS 0x43c04010 
#define UART54_D_SEND 0x43c04008 
#define UART54_D_RECV 0x43c04000 
//#define UART54_D_IRQR 0x43c50000 



//RGB LEDS
#define LED1_B  0x43c05000
#define LED1_G  0x43c05004
#define LED1_R  0x43c05008
#define LED2_B  0x43c0500c
#define LED2_G  0x43c05010
#define LED2_R  0x43c05014

//ZMQ data backend
#define EVLEN 8

int setCLKx2(int FkHz); // set PIXLAR CLOCKx2 output frequency, kHz
int rgb(int r1, int g1, int b1, int r2, int g2, int b2); //values are given in percents 0-100
int uart54_send(int chan, uint64_t *buf, int num); // send 54-bits word to channel chan (0->A, 1->B)
int uart54_recv(int chan, uint64_t *buf, int num); // blocks until receive requested num words
int uart54_available(int chan); //returns 1 if word is available in buffer, 0 otherwise
int uart54_getstats(int chan, uint64_t* starts, uint64_t* stops); //returns number of start and stop bits on the RX since last reset
int system_reset(); //issues system reset pulse for UART and PIXLAR asics

void dump(volatile unsigned char * ptr);
void dumpd(volatile uint64_t word);
void dumpc(volatile uint64_t word);
void dump_decoded(volatile uint64_t word);
uint64_t parity(uint64_t w);


