#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "pixlar.h"

int main() {

    uint64_t w;
//   if(uart54_available(0)) 
//    {
    uart54_recv(2, &w, 1);
    printf("C: 0x%0llx\n",w);
 //   }
/*
    off_t offset = UART54_A_RECV;
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
    while(mem[page_offset+len-1]<0x80) {}
    for (i = 0; i < len; ++i)
     //   printf("%08x ", (unsigned int)mem[page_offset]);
        printf("%02x", (unsigned int)mem[page_offset+len-i-1]);
        printf("\n");
    mem[page_offset+len-1]=0;
*/
    return 0;

}
