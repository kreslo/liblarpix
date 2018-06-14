#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

int main(int argc, char *argv[]) {

    if(argc!=7) return 0;
    int r1= strtof(argv[1],NULL); 
    int g1= strtof(argv[2],NULL); 
    int b1= strtof(argv[3],NULL); 
    int r2= strtof(argv[4],NULL); 
    int g2= strtof(argv[5],NULL); 
    int b2= strtof(argv[6],NULL); 

    return rgb(r1,g1,b1,r2,g2,b2);
/* 
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

    size_t i;
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
*/
  //  return 0;
}
