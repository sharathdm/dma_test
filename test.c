#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void main() {
        int fd;
        volatile char *ptr;
        unsigned char read_data[20];
        unsigned char write_data[10] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa};
        /*fd = open("/dev/hello", O_RDWR |  O_SYNC);*/
        fd = open("/dev/hello", O_RDWR );
        if(fd !=-1) {
                ptr = mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
                if(ptr == -1) {
                        printf("mmap error\n");
                        goto error;
                } else {
                        printf("%x %x %x %x\n",ptr[0], ptr[1], ptr[2], ptr[3]);
                        ptr[0] = 0xaa;
                        ptr[1] = 0xaa;
                        ptr[2] = 0xaa;
                        ptr[3] = 0xaa;
                        ptr[4] = 0xaa;
                        printf("%x %x %x %x\n",ptr[0], ptr[1], ptr[2], ptr[3]);
                        read(fd, read_data, 5);
                        printf("read %x %x %x %x %x\n",read_data[0], read_data[1], read_data[2], read_data[3], read_data[4]);
                        write(fd, write_data, 10);
                        printf("%x %x %x %x\n",ptr[0], ptr[1], ptr[2], ptr[3]);
                        read(fd, read_data, 5);
                        printf("read %x %x %x %x %x\n",read_data[0], read_data[1], read_data[2], read_data[3], read_data[4]);

                        munmap(ptr, 4096);
                }

        } else {
                printf("open error\n");
                return -1;
        }
error:
        close(fd);
}