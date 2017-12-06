
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>


char* buf;
struct stat statbuf;

void handler(int signo)
{
    printf("permission denied\n");
    
    if (mprotect(buf, statbuf.st_size, PROT_READ|PROT_WRITE) == -1)
    {
        perror("failed to alter permission");
        exit(1);
    }
    printf("permission modified\n");
}

int main()
{
    int fd;
    
    if (signal(SIGSEGV, handler) == SIG_ERR)
    {
        printf("can not set handler for SIGSEGV.\n");
        exit(0);
    }
    
    if (stat("test.txt", &statbuf) == -1)
    {
        perror("failed to get stat");
        exit(1);
    }
    
    fd = open("test.txt", O_RDWR);
    if (fd == -1)
    {
        perror("failed to open file\n");
        exit(1);
    }
    
    buf = (char*)mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);  // readonly memory map
    printf("try to write to a readonly memccpy map\n");
    memcpy(buf, "china\n", 6);
 
    if (munmap(buf, statbuf.st_size) == -1)
    {
        perror("failed to unmap\n");
        exit(1);
    }
    close(fd);
    
    return 0;
}
