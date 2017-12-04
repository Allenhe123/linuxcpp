#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

////未解决写锁饥饿问题。。。。

// 1. 用互斥锁和条件变量实现读写锁：
class readwrite_lock
{
public:
	readwrite_lock(): stat(0)
	{
	}

	void readLock()
	{
		mtx.lock();
		while (stat < 0)
			cond.wait(mtx);
		++stat;
		mtx.unlock();
	}

	void readUnlock()
	{
		mtx.lock();
		if (--stat == 0)
			cond.notify_one(); // 叫醒一个等待的写操作
		mtx.unlock();
	}

	void writeLock()
	{
		mtx.lock();
		while (stat != 0)
			cond.wait(mtx);
		stat = -1;
		mtx.unlock();
	}

	void writeUnlock()
	{
		mtx.lock();
		stat = 0;
		cond.notify_all(); // 叫醒所有等待的读和写操作
		mtx.unlock();
	}

private:
	mutex mtx;
	condition_variable cond;
	int stat; // == 0 无锁； > 0 已加读锁个数； < 0 已加写锁
};


/// 2. 使用2个互斥锁实现读写锁：
class readwrite_lock_2
{
public:
	readwrite_lock_2(): read_cnt(0)
	{
	}

	void readLock()
	{
		read_mtx.lock();
		if (++read_cnt == 1)
			write_mtx.lock();

		read_mtx.unlock();
	}

	void readUnlock()
	{
		read_mtx.lock();
		if (--read_cnt == 0)
			write_mtx.unlock();

		read_mtx.unlock();
	}

	void writeLock()
	{
		write_mtx.lock();
	}

	void writeUnlock()
	{
		write_mtx.unlock();
	}

private:
	mutex read_mtx;
	mutex write_mtx;
	int read_cnt; // 已加读锁个数
};

/// 3.用互斥锁mutex和条件变量conditon实现写优先的读写锁
class RWLock {
private:
    pthread_mutex_t mxt;
    pthread_cond_t cond;
    int rd_cnt;//等待读的数量
    int wr_cnt;//等待写的数量

public:
    RWLock() :rd_cnt(0), wr_cnt(0) {
        pthread_mutex_init(&mxt,NULL);
        pthread_cond_init(&cond,NULL);
    }

    void readLock() {
        pthread_mutex_lock(&mxt);
        ++rd_cnt;
        while(wr_cnt > 0)
            pthread_mutex_wait(&cond, &mxt);
		
        pthread_mutex_unlock(&mxt);
    }

    void readUnlock() {
        pthread_mutex_lock(&mxt);
        
        --rd_cnt;
        if (rd_cnt == 0 )
            pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mxt);
    }

    void writeLock() {
        pthread_mutex_lock(&mxt);

        ++wr_cnt;
        while (wr_cnt + rd_cnt >= 2)
            pthread_cond_wait(&cond, &mxt);

        pthread_mutex_unlock(&mxt);
    }

    void writerUnlock() {
        pthread_mutex_lock(&mxt);

        --wr_cnt;
        if(wr_cnt==0)
            pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mxt);
    }
};

// use case: 1.读频率远大于写频率 2.需要在较长时间持有锁的情况下
