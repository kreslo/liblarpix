#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"



int main(int argc, char *argv[]) {

    if(argc!=4) return 0;
    uint64_t chipid,reg,data,par;
    chipid=strtof(argv[1], NULL);
    reg=strtof(argv[2], NULL);
    data=strtof(argv[3], NULL);
    uint64_t wrd64= 0; //strtoull(argv[1], NULL, 0); 
    wrd64=2;
    wrd64=wrd64 | (chipid<<2);
    wrd64=wrd64 | (reg<<10);
    wrd64=wrd64 | (data<<18);
    par=parity(wrd64);
    wrd64=wrd64 | (par<<53);

    dumpc(wrd64);
    uart54_send(0, &wrd64, 1);
    uart54_send(1, &wrd64, 1);
    uart54_send(2, &wrd64, 1);
    uart54_send(3, &wrd64, 1);
    return 1;
   

/*
    off_t offset = UART54_B_SEND;
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
    while( *(mem+page_offset+7)<0x80) {}
    *((volatile uint64_t*)(mem+page_offset))=wrd64;

    return 0;
*/
}
