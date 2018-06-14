#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

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
    return 0;
}




int uart54_send(int chan, uint64_t *buf, int num)// send 54-bits word to channel chan (0->A, 1->B)
{
    off_t offset;
    if(chan==0) offset = UART54_A_SEND;
    else if(chan==1) offset = UART54_B_SEND;
    else return -1;
    size_t len = 8;

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

    size_t i;
    for( i=0; i<num; i++)
    {
      while( *(mem+page_offset+7)<0x80) {}
      *((volatile uint64_t*)(mem+page_offset))=buf[i];
    }
  munmap((void*)mem, page_offset + len);
  return 0;
} 
int uart54_recv(int chan, uint64_t *buf, int num) // blocks until receive requested num words
{
    off_t offset;
    if(chan==0) offset = UART54_A_RECV;
    else if(chan==1) offset = UART54_B_RECV;
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

    size_t i;
    for( i=0; i<num; i++)
     { 
      while(mem[page_offset+len-1]<0x80) {} //wait until word is available in UART: data_ready
      buf[i]=*(volatile uint64_t*)(mem+page_offset); 
      mem[page_offset+len-1]=0; //reset data_ready bit
     }
  munmap((void*)mem, page_offset + len);

  return num;
} 
int uart54_available(int chan) //returns 1 if word is available in buffer, 0 otherwise
{
    int retval;
    off_t offset;
    if(chan==0) offset = UART54_A_RECV;
    else if(chan==1) offset = UART54_B_RECV;
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

    return retval;
} 

int setCLKx2(int FkHz) // set PIXLAR CLOCKx2 output frequency, kHz
{
  
    off_t offset=CLOCKx2_DIVIDER;
    size_t len = 8;
    uint32_t div=5;
    float fresult;
    uint32_t FBASE=50000; // base frequency, kHz (50 MHz)
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


return 0;
}

