#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"


int main() {
   off_t offsetA = UART54_A_RECV;
    off_t offsetB = UART54_B_RECV;
    off_t offsetC = UART54_C_RECV;
    off_t offsetD = UART54_D_RECV;
    size_t len = 8;

    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_baseA = (offsetA / pagesize) * pagesize;
    off_t page_offsetA = offsetA - page_baseA;
    off_t page_baseB = (offsetB / pagesize) * pagesize;
    off_t page_offsetB = offsetB - page_baseB;
    off_t page_baseC = (offsetC / pagesize) * pagesize;
    off_t page_offsetC = offsetC - page_baseC;
    off_t page_baseD = (offsetD / pagesize) * pagesize;
    off_t page_offsetD = offsetD - page_baseD;

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

    volatile unsigned char *memC = mmap(NULL, page_offsetC + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_baseC);
    if (memC == MAP_FAILED) {
        perror("Can't map C memory");
        return -1;
    }

    volatile unsigned char *memD = mmap(NULL, page_offsetD + len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_baseD);
    if (memD == MAP_FAILED) {
        perror("Can't map D memory");
        return -1;
    }

    size_t i;
    
    while(1)
{
    if(memA[page_offsetA+len-1]>=0x80)
    {
    printf("A:");
    dump_decoded(*(volatile uint64_t*)(memA+page_offsetA));
    memA[page_offsetA+len-1]=0;
    } 
    if(memB[page_offsetB+len-1]>=0x80)
    {
    printf("B:");
    dump_decoded(*(volatile uint64_t*)(memB+page_offsetB));
    memB[page_offsetB+len-1]=0;
    } 
    if(memC[page_offsetC+len-1]>=0x80)
    {
    printf("C:");
    dump_decoded(*(volatile uint64_t*)(memC+page_offsetC));
    memC[page_offsetC+len-1]=0;
    } 
    if(memD[page_offsetD+len-1]>=0x80)
    {
    printf("D:");
    dump_decoded(*(volatile uint64_t*)(memD+page_offsetD));
    memD[page_offsetD+len-1]=0;
    } 
}


    return 0;
}


