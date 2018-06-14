#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

int main(int argc, char **argv) {

    uint64_t wrd64= 0; 
    off_t offset = UART54_A_SEND;
    size_t len = 8;
    int pseus=0;
    if(argc==2) pseus= strtof(argv[1],NULL);
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
    while(1)
{
    while( *(mem+page_offset+7)<0x80) {}
    *((volatile uint64_t*)(mem+page_offset))=wrd64;
    wrd64++;
    usleep(pseus);
}
    return 0;
}
