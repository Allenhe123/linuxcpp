/*  
// EXAMPLE 1   flock
#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <string.h> 
#include <sys/file.h> 
#include <wait.h>

#define COUNT 100 
#define NUM 64 
#define FILEPATH "/tmp/count" 

int do_child(const char *path) { 
    
    //* 当多个进程同时执行这个过程的时候，就会出现racing：竞争条件，
    //* 多个进程可能同时从文件独到同一个数字，并且分别对同一个数字加1并写回， 
    // 导致多次写回的结果并不是我们最终想要的累积结果。

    int fd; 
    int ret, count; 
    char buf[NUM]; 
    fd = open(path, O_RDWR);    // 1. 打开FILEPATH路径的文件
    flock(fd, LOCK_EX);         // 对整个文件加锁 
    ret = read(fd, buf, NUM);   // 2. 读出文件中的当前数字
    buf[ret] = '\0'; 
    count = atoi(buf);                        // 3. 将字符串转成整数
    ++count;                                    // 4. 整数自增加1 
    sprintf(buf, "%d", count);                 // 5. 将整数转成字符串 
    lseek(fd, 0, SEEK_SET);                    // 6. lseek调整文件当前的偏移量到文件头  
    ret = write(fd, buf, strlen(buf));         // 7. 将字符串写会文件 
    flock(fd, LOCK_UN);                      // 解锁 
    close(fd); 
    exit(0);
} 

int main() { 
    pid_t pid; 
    int count; 
    for (count=0; count<COUNT; count++)
    {
        pid = fork(); 

        if (pid == 0) 
        {
            do_child(FILEPATH);
        }
    } 
    for (count=0; count<COUNT; count++) 
    { 
        wait(NULL); 
    } 
}
*/



/*
// EXAMPLE 2   flock
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <wait.h>

#define PATH "/tmp/lock"

int main()
{
    int fd;
    pid_t pid;

    fd = open(PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);

    flock(fd, LOCK_EX); // 对整个文件加锁

    printf("%d: locked!\n", getpid());

    pid = fork();

    if (pid == 0) {
        
        fd = open(PATH, O_RDWR|O_CREAT|O_TRUNC, 0644);  // MUST重新打开文件 !!!
        
        flock(fd, LOCK_EX); // 加锁
        printf("%d: locked!\n", getpid());
        exit(0);
    }
    wait(NULL);
    unlink(PATH);
    exit(0);
}
*/


/*
//EXAMPLE 3     lockf

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <wait.h>

#define PATH "/tmp/lock"

int main()
{
    int fd;
    pid_t pid;
    fd = open(PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    lockf(fd, F_LOCK, 0);
    printf("%d: locked!\n", getpid());
    pid = fork();

    if (pid == 0) { 
        // 在子进程不用open重新打开文件的情况下，进程执行仍然被阻塞在子进程lockf加锁的操作上
        //fd = open(PATH, O_RDWR|O_CREAT|O_TRUNC, 0644);
        lockf(fd, F_LOCK, 0);
        printf("%d: locked!\n", getpid());
        exit(0);
    }
    wait(NULL);
    unlink(PATH);
    exit(0);
}
*/


//EXAMPLE 4     fcntl
// fcntl这个函数对文件进行加锁
// 加了读锁的话,不能加写锁,但可以继续加读锁.
// 加了写锁的话,不能加读锁,同时也不能继续加写锁了,除非这个写锁释放.

// struct flock
// l_type可以取3种值, F_RDLCK表示读锁,F_WRLCK表示写锁,F_UNLCK表示解锁.
// l_start伴随着l_whence来解释:
// SEEK_SET:l_start相对于文件的开头解释.
// SEEK_CUR:l_start相对于文件的当前字节偏移(即当前的读写指针的位置)解释.
// SEEK_END: l_start相对于文件的末尾解释.
// l_len成员指定从该偏移开始的连续字节数,长度为0代表"从起始偏移到文件偏移的最大可能值".因此锁住整个文件有两种方式:
// 指定l_whence成员为SEEK_SET, l_start成员为0,l_len成员为0.
// 使用lseek将读写指针定位到文件头,然后指定l_whence成员为SEEK_CUR, l
// _start为0,l_len为0.

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <wait.h>
#include <string.h>
#define PATH "/tmp/count"

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {
    struct flock lock;
    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;
    return (fcntl(fd, cmd, &lock));
}

#define read_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)

//阻塞版本的读锁
//F_SETLKW F_SETLK 作用相同，但是无法建立锁定时，此调用会一直等到锁定动作成功为止。若在等待锁定的过程中被信号中断时，会立即返回-1，错误代码为EINTR。
#define readw_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define write_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)

// 阻塞版本的写锁
#define writew_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define un_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)


int main(int argc, char *argv[])
{
    int fd = open(PATH, O_RDONLY, NULL); // 打开文件

    readw_lock(fd, 0, SEEK_SET, 0);      // 加读锁

    char buf[512] = { 0 };
    read(fd, buf, sizeof(buf));
    printf("%s\n", buf);
    sleep(10);
    un_lock(fd, 0, SEEK_SET, 0);      // 解锁

    return 0;
}
