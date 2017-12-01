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
#define LOCK_NAME "/tmp/.filelock"

//g++ process_lock2.cpp -o plock2

//////has issue.....?!
class Filelock
{
public:
	Filelock(const char* filename): lockfile(-1) 
	{
		strcpy(name, filename);
	}
	
	~Filelock()
	{
		printf("### call destroyed\n");
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
		printf("### call uninit\n");
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
		printf("@@@ lockfile=%d\n", lockfile);
		assert(lockfile != -1);
		int ret = lockf(lockfile, F_LOCK, 0);
		if (ret == 0) return true;
		else 
		{
			printf("lock failed error = %d", errno);
			return false;
		}
	}
	
	bool unlock() 
	{
		printf("@@@ lockfile=%d\n", lockfile);
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
	int pid = 0;
	int ret = 0;
	char* shmaddr = 0;
	
	int shm_id = shmget(IPC_PRIVATE, SIZE, IPC_CREAT|0600);
	assert(shm_id != -1);

	int fd = open(LOCK_NAME, O_CREAT|O_WRONLY, 0666);
	assert(fd > 0);
	
	pid = fork(); 
	if (pid == 0)
	{   
		shmaddr = (char*)shmat(shm_id, NULL, 0);
		//assert((int)shmaddr != -1);

		int* x = (int*)shmaddr; 
									        
		for (int i=0; i<10; i++) 
		{
			int ret = lockf(fd, F_LOCK, 0);
			
			(*x)++;
			printf("x++: %d\n", *x);
			//lock.unlock();  
			ret = lockf(fd, F_ULOCK, 0);
			assert(ret == 0);   
			sleep(1);                      
		}

		ret = shmdt(shmaddr);
		assert(ret == 0);
		// delete this share memory in main process, so here we don't call delete!
//		ret = shmctl(shm_id, IPC_RMID, NULL);
//		assert(ret == 0);
	}        
	else
	{	
		shmaddr = (char*)shmat(shm_id, NULL, 0);
		int* x = (int*)shmaddr;

		for (int i=0; i<10; i++)
		{
			int ret = lockf(fd, F_LOCK, 0);
			assert(ret == 0);

			*x += 2;
			printf("x+=2: %d\n", *x);
			ret = lockf(fd, F_ULOCK, 0);
			assert(ret == 0);
			sleep(1);
		}

		ret = shmdt(shmaddr);
		assert(ret == 0);
		ret = shmctl(shm_id, IPC_RMID, NULL);
		assert(ret == 0);
	}
	
	return(0);
}
