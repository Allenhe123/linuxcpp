#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
using namespace std;


//读写锁非常适合读数据的频率远大于写数据的频率从的应用中。这样可以在任何时刻运行多个读线程并发的执行，给程序带来了更高的并发度。

//排队的线程中有等写锁的线程时，申请读锁会阻塞（##写锁优先##）。

struct 
{
	pthread_rwlock_t rwlock;
	int product;
} sharedData; // = { PTHREAD_RWLOCK_INITIALIZER, 0 };

void* produce(void* ptr)
{
	for (int i = 0; i < 5; ++i)
	{
		//加写锁
		pthread_rwlock_wrlock(&sharedData.rwlock);
		sharedData.product = i;
		pthread_rwlock_unlock(&sharedData.rwlock);
		sleep(1);
	}
}

void * consume1(void *ptr)
{
	for (int i = 0; i < 5; ++i)
	{
		//加读锁
		pthread_rwlock_rdlock(&sharedData.rwlock);
		cout << "consume1, index: "<< i << " producer: " << sharedData.product << endl;
		pthread_rwlock_unlock(&sharedData.rwlock);
		sleep(1);
	}
}

void * consume2(void *ptr)
{
	for (int i = 0; i < 5; ++i)
	{
		//加读锁
		pthread_rwlock_rdlock(&sharedData.rwlock);
		cout << "consume2, index: " << i <<" producer: " << sharedData.product << endl;
		pthread_rwlock_unlock(&sharedData.rwlock);
		sleep(1);
	}
}

int main()
{
	pthread_rwlock_init(&sharedData.rwlock, NULL);   //初始化读写锁
	
	pthread_t threads[3];
	int ret = 0;
	ret = pthread_create(&threads[0], NULL, produce, NULL);  assert(ret == 0);
	ret = pthread_create(&threads[1], NULL, consume1, NULL);  assert(ret == 0);
	ret = pthread_create(&threads[2], NULL, consume2, NULL);  assert(ret == 0);

	void* retVal[3];
	for (int i=0; i<3; i++)
	{
/*
在很多情况下，主线程生成并起动了子线程，如果子线程里要进行大量的耗时的运算，主线程往往将于子线程之前结束，
但是如果主线程处理完其他的事务后，需要用到子线程的处理结果，也就是主线程需要等待子线程执行完成之后再结束，
这个时候就要用到pthread_join()方法了。
即pthread_join()的作用可以这样理解：主线程等待子线程的终止。
也就是在子线程调用了pthread_join()方法后面的代码，只有等到子线程结束了才能执行。
*/
		ret = pthread_join(threads[i], &retVal[i]); //assert(ret == 0);
		if(ret != 0)  
        {  
            if(ret == ESRCH)  
            {  
                cerr << "pthread_join():ESRCH 没有找到与给定线程ID相对应的线程" << endl;  
            }  
            else if(ret == EDEADLK)  
            {  
                cerr << "pthread_join():EDEADLKI 产生死锁" << endl;  
            }  
            else if(ret == EINVAL)  
            {  
                cerr << "pthread_join():EINVAL 与给定的县城ID相对应的线程是分离线程" << endl;  
            }  
            else  
            {  
                cerr << "pthread_join():unknow error" << endl;  
            }  
            exit(-1);
		}
	}
	pthread_rwlock_destroy(&sharedData.rwlock);      //销毁读写锁 
	return 0;
}
