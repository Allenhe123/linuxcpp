#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/file.h>  
#include <stdio.h>  
  
int main()  
{  
    int fd;  
      
    printf("try_lock============================\n");  
    fd = open("lock.txt", O_RDONLY);  
    if (-1 == fd)  
    {  
        printf("open() failed.\n");  
        return -1;  
    }  
      
    printf("try lock!\n");  
    if (-1 == flock(fd, LOCK_EX | LOCK_NB))  
    {  
        printf("flock() failed.\n");  
        return -1;  
    }  
      
    if (-1 == close(fd))  
    {  
        printf("close() failed.\n");  
        return -1;  
    }  
      
    return 0;  
}  