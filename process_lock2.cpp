#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/mman.h>   
#include <unistd.h>
#include <pthread.h>   
#include <stdio.h> 
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

#define SIZE 4
#define LOCK_NAME "filelock.dat"

//g++ process_lock2.cpp -o plock2

class Filelock
{
public:
	Filelock(const char* filename): lockfile(-1) 
	{
		strcpy(name, filename);
	}
	
	~Filelock()
	{
		if (lockfile != -1)
		{
			close(lockfile);
			lockfile = -1;
		}
	}
	
	bool init(bool& exist)
	{
		bool bret = false;
		int fd = -1;
		assert(strlen(name) > 0);

		fd = open(name, O_CREAT|O_EXCL|O_WRONLY, 0666);
		if (fd == -1 && errno == EEXIST)
		{
			exist = true;
			fd = open(name, O_CREAT|O_WRONLY, 0666);
		}
		
		if (fd == -1)
		{
			printf("umutexproc create mutex failed error = %d", errno);
			bret = false;
		}
		else
		{	
			lockfile = fd;
			bret = true;
		}
		return bret;
	}
	
	void uninit()
	{
		bool locked = getstatus();
		if (lockfile != -1)
		{
			close(lockfile);
			lockfile = -1;
		}

		if (!locked)
		{	
        /***
         *  There is a potential risk on more than 2 processes which use the same mutex and one of them try to uninit.
         *  While A is trying to uninit the mutex after getstatus with unlocked, that means the lock will be destroyed.
         *  B already got the lock, B work well.
         *  Then C will open the lock with the same name. Due to the file was removed, C is able to get a new lock with the same name as B.
         *  The mutex work well for syncing sign between processes with lock and unlock.
         *  The mutex have chance to be invalid with incorrect uninit.
         *  It's suggestion that uninit all the mutex after all the sync job finished.
         */
			unlink(name); /// unlink will remove the file and destroy the mutex.
		}
		name[0] = '\0';
	}
	
	bool getstatus() 
	{
		assert(lockfile != -1);
		int ret = lockf(lockfile, F_TEST, 0);
		if (ret == -1 && (errno == EAGAIN || errno == EACCES))
			return true;
		else return false;
	}
	
	bool trylock() 
	{
		assert(lockfile != -1);
		int ret = lockf(lockfile, F_TLOCK, 0);
		if (ret == 0) return true;
		else 
		{	printf("trylock failed error = %d", errno);
			return false;
		}
	}
	
	bool lock() 
	{
		assert(lockfile != -1);
		int ret = lockf(lockfile, F_LOCK, 0);
		if (ret == 0) return true;
		else 
		{
			printf("lock failed error = %d", errno);
			return false;
		}
	}
	
	//bool tryunlock() {}
	bool unlock() 
	{
		assert(lockfile = -1);
		int ret = lockf(lockfile, F_ULOCK, 0);
		if (ret == 0)
			return true;
		else
		{
			printf("umutexproc unlock failed error = %d", errno);
			return false;
		}
	}
	
private:
	Filelock& operator = (const Filelock&);
	Filelock(const Filelock&);
	
	char name[256];
	int lockfile;
};

int main()
{
#if _POSIX_SHARED_MEMORY_OBJECTS
	printf("Support this kind of process lock\n");
#else
	printf("Do not support this kind of process lock\n");
	return 0;
#endif

	int pid = 0;     
	int shm_id = 0;
	int ret = 0;
	struct shmid_ds buf;
	int* x = NULL;     
	char* shmaddr = NULL;
	char* addnum = "myadd";     
	void* ptr = NULL; 
	    

	Filelock lock(LOCK_NAME);
	bool exist = false;
	lock.init(exist);
	
	// 得到一个共享内存标识符或创建一个共享内存对象
	shm_id = shmget(IPC_PRIVATE, SIZE, IPC_CREAT|0600);
	assert(shm_id != -1);
	
	pid = fork(); // 复制父进程，并创建子进程  
	    
	if (pid == 0)  //子进程完成x+1  
	{   
		// 打开或创建一个共享内存区, hm_open的返回值是一个整数描述字，它随后用作mmap的第五个参数
		//shm_id = shm_open(addnum, O_RDWR, 0);     
		//ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0); // 连接共享内存
		// 把共享内存区对象映射到调用进程的地址空间
		shmaddr = (char*)shmat(shm_id, NULL, 0);
		//assert((int)shmaddr != -1);

		x = (int *)shmaddr;       
										        
		for (int i=0; i<10; i++)
		{  
			while (1)
			{
				bool b = lock.lock();
				if (!b)
					sleep(100);
				else
					break;
			}
			
			(*x)++;
			printf("x++: %d\n", *x);
			lock.unlock();     
			sleep(1);                      
		}
		// 断开与共享内存附加点的地址，禁止本进程访问此片共享内存
		ret = shmdt(shmaddr);
		assert(ret == 0);
	}        
	else  // 父进程完成x+2
	{     
		//shm_id = shm_open(addnum, O_RDWR | O_CREAT, 0644);
		//ftruncate(shm_id, sizeof(int));
		//ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);// 连接共享内存区
		// 共享内存管理
		int flag = shmctl( shm_id, IPC_STAT, &buf);
		assert(flag != -1);
		printf("shm_segsz =%d bytes\n", buf.shm_segsz );
		printf("parent pid=%d, shm_cpid = %d \n", getpid(), buf.shm_cpid );
		printf("chlid pid=%d, shm_lpid = %d \n", pid , buf.shm_lpid );

		shmaddr = (char*)shmat(shm_id, NULL, 0);
		//assert((int)shmaddr != -1);

		x = (int *)shmaddr;
					                 
		for (int i=0; i<10; i++)
		{
			while (1)
			{
				bool b = lock.lock();
				if (!b)
					sleep(100);
				else
					break;
			}
			//lock.lock();          
			(*x) += 2;
			printf("x+=2: %d\n",*x);
			lock.unlock();           
			sleep(1);      
		}
		ret = shmdt(shmaddr);
		assert(ret == 0);
		ret = shmctl(shm_id, IPC_RMID, NULL);
		assert(ret == 0);		
	}
	
	lock.uninit();
	return(0);
}
