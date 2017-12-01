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

#define SIZE 4

// build: gcc -std=c99 -pthread  process_lock.c -o plock
//2个进程，一个进程完成每次加1,另一个进程完成每次加2,2个进程协作完成累加,使用共享内存方式在进程间通信

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
	    
	pthread_mutex_t mutex;               // 互斥对象     
	pthread_mutexattr_t mutexattr;       // 互斥对象属性
	ret = pthread_mutex_init(&mutex, NULL);
	assert(ret == 0);	
	ret = pthread_mutexattr_init(&mutexattr);  // 初始化互斥对象属性
	assert(ret == 0);
	// 设置互斥对象为PTHREAD_PROCESS_SHARED共享，即可以在多个进程的线程访问,PTHREAD_PROCESS_PRIVATE为同一进程的线程共享 	
	ret = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
	assert(ret == 0);
	// 在创建 pthread mutex 的时候，指定为 ROBUST 模式
	//ret = pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
	assert(ret == 0);
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
		assert((int)shmaddr != -1);

		x = (int *)shmaddr;       
										        
		for (int i=0; i<10; i++)
		{  
			pthread_mutex_lock(&mutex);           
			(*x)++;
			printf("x++: %d\n", *x);
			pthread_mutex_unlock(&mutex);      
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
		assert((int)shmaddr != -1);

		x = (int *)shmaddr;
					                 
		for (int i=0; i<10; i++)
		{
			pthread_mutex_lock(&mutex);     
			(*x) += 2;
			printf("x+=2: %d\n",*x);
			pthread_mutex_unlock(&mutex);       
			sleep(1);      
		}
		ret = shmdt(shmaddr);
		assert(ret == 0);
		ret = shmctl(shm_id, IPC_RMID, NULL);
		assert(ret == 0);		
	}
	
	//pthread_mutexattr_destory(&mutexattr);
	//pthread_mutexattr_destory(&mutexattr);
	//shm_unlink(addnum);      //删除共享名称     
	//munmap(ptr, sizeof(int));//删除共享内存
	return(0);

}

/*
posix共享内存shm_open 和 systemV共享内存shmget 方式

实现进程互斥锁的几个常见方案：Posix信号量、System V信号量以及线程锁共享，并且分析了他们的平台兼容性以及严重缺陷。这里要介绍
一种安全且平台兼容的进程互斥锁，它是基于文件记录锁实现的。


*/
