#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdio.h>  
  
#define FILE_APPENDED_CONTENTS "123"  
  
int main()  
{  
    int fd;  
      
    printf("append============================\n");  
    fd = open("lock.txt", O_RDWR | O_APPEND);  
    if (-1 == fd)  
    {  
        printf("open() failed.\n");  
        return -1;  
    }  
      
    printf("append something to lock.txt!\n");  
    if (-1 == write(fd, FILE_APPENDED_CONTENTS, sizeof(FILE_APPENDED_CONTENTS)))  
    {  
        printf("write() failed.\n");  
        return -1;  
    }  
      
    if (-1 == close(fd))  
    {  
        printf("close() failed.\n");  
        return -1;  
    }  
      
    return 0;  
}  