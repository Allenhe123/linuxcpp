#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/file.h>  
#include <stdio.h>  
  
#define FILE_CONTENTS "123"  
  
int main()  
{  
    int fd;  
      
    if (0 == access("lock.txt", F_OK))  
    {  
        printf("remove lock.txt!\n");  
        if (-1 == remove("lock.txt"))  
        {  
            printf("remove() failed.\n");  
            return -1;  
        }  
    }  
      
    fd = open("lock.txt", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRUSR | S_IWUSR);  
    if (-1 == fd)  
    {  
        printf("open() failed.\n");  
        return -1;  
    }  
      
    if (-1 == write(fd, FILE_CONTENTS, sizeof(FILE_CONTENTS)))  
    {  
        printf("write() failed.\n");  
        return -1;  
    }  
      
    if (-1 == flock(fd, LOCK_EX))  
    {  
        printf("flock() failed.\n");  
        return -1;  
    }  
      
    sleep(60);  
      
    if (-1 == close(fd))  
    {  
        printf("close() failed.\n");  
        return -1;  
    }  
      
    return 0;  
}  