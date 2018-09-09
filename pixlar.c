#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

int fd_led=0;

void dump(volatile unsigned char * ptr)
{
   int i;
     for (i = 0; i < 8; ++i)
        printf("%02x", *(unsigned char*)(ptr+7-i));
        printf("\n");
 
}

void dump_decoded(volatile uint64_t word)
{
  if((word & 0x0000000000000003) == 0) {printf("DATA "); dumpd(word); }
  if((word & 0x0000000000000003) == 2) {printf("CONF_W "); dumpc(word); }
  if((word & 0x0000000000000003) == 1) {printf("TEST "); dumpd(word); }
  if((word & 0x0000000000000003) == 3) {printf("CONF_R "); dumpc(word); }
}


void dumpd(volatile uint64_t word)
{
   int bc=0;
      printf("(");
      for (bc = 0; bc < 64; bc++)
     { 
       if(bc==2) printf(") id:");
       if(bc==10) printf(" ch:");
       if(bc==17) printf(" ts:");
       if(bc==41) printf(" adc:");
       if(bc==51) printf(" fifo/2:");
       if(bc==52) printf(" ovfl:");
       if(bc==53) printf(" par:");
       if(bc==54) printf(" ");
       if( ((word>>bc) & 1) > 0 ) printf("1"); else printf("0");
      
     }
     //   printf("%02x", *(unsigned char*)(ptr+7-i));
        printf("\n");
 
}

void dumpc(volatile uint64_t word)
{
   int bc=0;
      printf("(");

      for (bc = 0; bc < 64; bc++)
     { 
       if(bc==2) printf(") id:");
       if(bc==10) printf(" addr:");
       if(bc==18) printf(" data:");
       if(bc==26) printf(" zeros:");
       if(bc==53) printf(" parity:");
       if(bc==54) printf(" ");
       if( ((word>>bc) & 1) > 0 ) printf("1"); else printf("0");
      
     }
     //   printf("%02x", *(unsigned char*)(ptr+7-i));
        printf("\n");
 
}





int rgb(int r1, int g1, int b1, int r2, int g2, int b2)
{

    off_t offset = LED1_B;
    size_t len = 32;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    volatile uint8_t *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

    unsigned int val;
    val=0xFFFF*b1/100;
    *(volatile uint16_t*)(mem+page_offset)=(uint16_t)val;
    val=0xFFFF*g1/100;
    *(volatile uint16_t*)(mem+page_offset+4)=(uint16_t)val;
    val=0xFFFF*r1/100;
    *(volatile uint16_t*)(mem+page_offset+8)=(uint16_t)val;
    val=0xFFFF*b2/100;
    *(volatile uint16_t*)(mem+page_offset+12)=(uint16_t)val;
    val=0xFFFF*g2/100;
    *(volatile uint16_t*)(mem+page_offset+16)=(uint16_t)val;
    val=0xFFFF*r2/100;
    *(volatile uint16_t*)(mem+page_offset+20)=(uint16_t)val;
    munmap((void*)mem, page_offset + len);
    close(fd);
    return 0;
}





int uart54_send(int chan, uint64_t *buf, int num)// send 54-bits word to channel chan (0->A, 1->B)
{
    //int rc;
    if(chan>3 || chan<0) return -1;
    char str[128];
    sprintf(str,"/dev/uart64%d",chan);
    int fd = open(str, O_RDWR); if(fd<=0) {printf("Can't open %s. Returning -1.",str); return -1;}
//    else printf("Opened %s. fd=%d",str,fd); 
    size_t i;
      write(fd,buf,num*8);
    close(fd);
  return 0;
} 

int uart54_recv(int chan, uint64_t *buf, int num) // blocks until receive requested num words
{

    if(chan>3 || chan<0) return -1;
    char str[128];
    sprintf(str,"/dev/uart64%d",chan);
    int fd = open(str, O_RDWR); if(fd<=0) return -1;
    int recvd=0;
      while(recvd<num*8)
      recvd=recvd+read(fd,&buf[recvd],num*8);
    close(fd);
  return num;
} 


int uart54_getstats(int chan,  uint64_t* starts, uint64_t* stops) //returns number of start bits on the RX since last reset
{
    volatile uint64_t retval;
    off_t offset;
    if(chan==0) offset = UART54_A_STARTS;
    else if(chan==1) offset = UART54_B_STARTS;
    else if(chan==2) offset = UART54_C_STARTS;
    else if(chan==3) offset = UART54_D_STARTS;
    else return -1;

    size_t len = 16;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
//    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

    *starts=*(volatile uint64_t*)(mem+page_offset);
    *stops=*(volatile uint64_t*)(mem+page_offset+8);
    munmap((void*)mem, page_offset + len);
    close(fd);
    retval=1;
    return retval;
} 

uint64_t uart54_getstops(int chan) //returns number of stop bits on the RX since last reset
{
    volatile uint64_t retval;
    off_t offset;
    if(chan==0) offset = UART54_A_STOPS;
    else if(chan==1) offset = UART54_B_STOPS;
    else if(chan==2) offset = UART54_C_STOPS;
    else if(chan==3) offset = UART54_D_STOPS;
    else return -1;

    size_t len = 8;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
//    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

    retval=*(volatile uint64_t*)(mem+page_offset);
    munmap((void*)mem, page_offset + len);
    close(fd);

    return retval;
} 


int uart54_available(int chan) //returns 1 if word is available in buffer, 0 otherwise
{
    int retval;
    off_t offset;
    if(chan==0) offset = UART54_A_RECV;
    else if(chan==1) offset = UART54_B_RECV;
    else if(chan==2) offset = UART54_C_RECV;
    else if(chan==3) offset = UART54_D_RECV;
    else return -1;

    size_t len = 8;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
//    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

    if(mem[page_offset+len-1]<0x80) retval=0;
    else retval=0;
    munmap((void*)mem, page_offset + len);
    close(fd);

    return retval;
} 

int setCLKx2(int FkHz) // set PIXLAR CLOCKx2 output frequency, kHz
{
  
    off_t offset=CLOCKx2_DIVIDER;
    size_t len = 8;
    uint32_t div=5;
    float fresult;
    uint32_t FBASE=125000; // base frequency, kHz (50 MHz)
    div=FBASE/FkHz;
    if(div<1) return -1;
    fresult=FBASE/div;
    printf("Frequency divider set to %d, CLOCKx2=%f kHz\n",div,fresult);
    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

      *((volatile uint32_t*)(mem+page_offset))=div-1;
  munmap((void*)mem, page_offset + len);
     close(fd);
 return 0;

}

int system_reset() //issues system reset pulse for UART and PIXLAR asics
{
    off_t offset=SYSTEM_RESET;
    size_t len = 8;
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    volatile unsigned char *mem = mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return -1;
    }

      *((volatile uint32_t*)(mem+page_offset))=1;
      usleep(1000);
      *((volatile uint32_t*)(mem+page_offset))=0;
  munmap((void*)mem, page_offset + len);
    close(fd);


return 1;
}

uint64_t parity(uint64_t w)
{
  uint64_t retval=1;
  for( int i=0; i<64; i++) retval=retval+((w>>i) & 0x1);
  retval=retval & 0x1;
  return retval;
}

