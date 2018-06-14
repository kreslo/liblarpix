#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

void dump(volatile unsigned char * ptr)
{
   int i;
     for (i = 0; i < 8; ++i)
        printf("%02x", *(unsigned char*)(ptr+7-i));
        printf("\n");
 
}

int main() {
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

    size_t i;
    
    while(1)
{
    if(memA[page_offsetA+len-1]>=0x80)
    {
    printf("A:");
    dump(memA+page_offsetA);
    memA[page_offsetA+len-1]=0;
    } 
    if(memB[page_offsetB+len-1]>=0x80)
    {
    printf("B:");
    dump(memB+page_offsetB);
    memB[page_offsetB+len-1]=0;
    } 
}


    return 0;
}


