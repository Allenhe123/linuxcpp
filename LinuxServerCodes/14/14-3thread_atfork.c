#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>

pthread_mutex_t mutex;

void* another( void* arg )
{
    printf( "in child thread, lock the mutex\n" );
    pthread_mutex_lock( &mutex );
    sleep( 5 );
    pthread_mutex_unlock( &mutex );
}

void prepare()
{
    pthread_mutex_lock( &mutex );
}

void infork()
{
    pthread_mutex_unlock( &mutex );
}

int main()
{
    pthread_mutex_init( &mutex, NULL );
    pthread_t id;
    pthread_create( &id, NULL, another, NULL );
    pthread_atfork( prepare, infork, infork );
    sleep( 1 );
    int pid = fork();
    if( pid < 0 )
    {
    	printf( "I should not run to here, holy shit...\n" );
        pthread_join( id, NULL );
        pthread_mutex_destroy( &mutex );
        return 1;
    }
    else if( pid == 0 )
    {
        printf( "I anm in the child, want to get the lock\n" );
        pthread_mutex_lock( &mutex );
        printf( "I can not run to here, oop...\n" );
        pthread_mutex_unlock( &mutex );
        exit( 0 );
    }
    else
    {
    	printf( "I am parent  process...\n" );
        pthread_mutex_unlock( &mutex );
        wait( NULL );
    }
    printf( "I am parent  process last ...\n" );
    pthread_join( id, NULL );
    pthread_mutex_destroy( &mutex );
    return 0;
}
