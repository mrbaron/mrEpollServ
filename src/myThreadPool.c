/*
 * myThreadPool.c
 *
 *  Created on: May 29, 2016
 *      Author: mrbaron
 */
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "mrepoll.h"
/*
 * myPool.h
 *
 * */


pthread_key_t key;
pthread_mutex_t * addMutex;
ptheadpool_error_t cur_error_t = threadpool_normal;

void hello_pool(threadpool_t * pool,int loop)
{
	//pthread_mutex_lock(addMutex);
	static unsigned ll = 1 ;
	unsigned long li = 0,la,lb,lc,le,ld,ii = 0 ;


	do{
		li = pthread_self()%10;
		la = pthread_self()%10;
		lb = pthread_self()%10;
		ll = pthread_self()%10;

		lc = pthread_self()%10;
		ld = pthread_self()%10;
		le = pthread_self()%10;
	}while(ii++<100000);
	printf("Thread %lu is leaving,li-e = %lu.%lu.%lu.%lu.%lu.%lu.ll = %lu\n",pthread_self(),li,la,lb,lc,ld,le,ll);
	//pthread_mutex_unlock(addMutex);
}


static void * threadpool_thread(void * threadpool_p)
{
	static int call_count =0,cur_pthread_pos = 0 ;
    threadpool_t * pool = threadpool_p ;
	threadpool_task_t task ;
	printf("thread called %d times.\n",call_count++);
	for(;;)
	{
		pthread_mutex_lock(&pool->mutex_lock);

		while((pool->count == 0) && (!pool->shutdown)) {
		            pthread_cond_wait(&(pool->notify), &(pool->mutex_lock));
		        }
		if(cur_error_t == threadpool_shutdown)
		{
			printf("threadpool_shutdown thread %lu!.\n",pthread_self());
			break;
		}
		printf("[threadpool_thread] count = %d.\n",pool->count);
		task.function = pool->queue->function;
		task.arg = pool->queue->arg;  //conn fd
		task.arg2 = pool->queue->arg2;
		task.arg3 = pool->queue->arg3;
		pool->head+=1;
		pool->head = (pool->head == pool->queue_count) ? 0 : pool->head;
		pool->count -= 1;

		pthread_mutex_unlock(&pool->mutex_lock);
		(*task.function)(task.arg,task.arg2,task.arg3);
		 //printf("finish a job %d,count %d\n",pool->head,pool->count);
	}
	pthread_mutex_unlock(&pool->mutex_lock);
	printf("thread %lu exit!.\n",pthread_self());
	pthread_exit(NULL);
	return NULL;
}

int threadpool_add(threadpool_t *pool ,void (*function) (void*),int arg,struct epoll_event * arg2,mrepoll_server_t * arg3)
{

	/*notice the status of pool*/
	if( pool->count >= pool->queue_count)
		return -1;
	if(pthread_mutex_lock(&(pool->mutex_lock)) != 0) {
	        return threadpool_lock_failure;
	}
	pool->queue[pool->count].function = function;
	pool->queue[pool->count].arg = arg;
	pool->queue[pool->count].arg2 = arg2;
	pool->queue[pool->count].arg3 = arg3;
	pool->count++;
	if(pthread_cond_signal(&(pool->notify)) != 0) {
		cur_error_t = threadpool_lock_failure;
	}
	if(pthread_mutex_unlock(&pool->mutex_lock) != 0) {
	        return threadpool_lock_failure;
	}
	return 0;
}

int shutdownPool(threadpool_t * pool)
{
	pool->shutdown = 1;
	if(cur_error_t == threadpool_normal)
	{
		if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
		           (pthread_mutex_unlock(&(pool->mutex_lock)) != 0)) {
				 	 cur_error_t = threadpool_lock_failure;
		        }
		cur_error_t = threadpool_shutdown;
		return 0;
	}
	return -1;
}

int waitThreadJoin(threadpool_t * pool)
{
	int i = 0  ;  void * result ;
	for( i = 0 ; i < pool->thread_count ; i++)
	{
		pthread_join(pool->threads[i],&result);
		printf("Joined with thread %d; returned value was %s\n",
				i, (char *) result);
		free(result);      /* Free memory allocated by thread */
	}
	cur_error_t = threadpool_joined;
	return 0;
}

int destroyThreadPool(threadpool_t * pool)
{
	 if(cur_error_t == threadpool_normal)
	 {

		 waitThreadJoin(pool);
	 }

	free(pool->attr);
	free(pool->pthread_ids);
	free(pool->saveflag);
	free(pool->threads);
	free(pool->queue);
	free(pool);
	return 0;
}

threadpool_t * createThreadPool(int maxPthread,int maxQueue)
{
	int i = 0 ,s = 0 ,stack_size =1024;
	if(maxPthread > MAX_THREADS || maxQueue > MAX_QUEUE)
	{
		return NULL;
	}
	/* alloc memory for threadpool_p */
	threadpool_t * threadpool_p = ( threadpool_t *)malloc(sizeof(threadpool_t));
	/*	Initialize arguments*/
	threadpool_p->queue_count = maxQueue;
	threadpool_p->thread_count = maxPthread;
	threadpool_p->head = 0;
	threadpool_p->tail = 0;
	threadpool_p->start = threadpool_p->shutdown = threadpool_p->count = 0;
	threadpool_p->attr = (pthread_attr_t *)malloc (sizeof(pthread_attr_t ));
	threadpool_p->pthread_ids = (int *)calloc(maxPthread,sizeof(int ));
	threadpool_p->saveflag = (int *)calloc(maxPthread,sizeof(int ));
	/*calloc memory for threads */
	threadpool_p->threads = (pthread_t * )malloc(maxPthread*sizeof(pthread_t));
	threadpool_p->queue = (threadpool_task_t *)malloc(maxQueue*sizeof(threadpool_task_t));
	s = pthread_attr_init(threadpool_p->attr);

	if((pthread_mutex_init(&threadpool_p->mutex_lock,NULL)!=0) ||
			(pthread_cond_init(&(threadpool_p->notify), NULL) != 0) ||
		       (threadpool_p->threads == NULL) ||
		       (threadpool_p->queue == NULL)) {
				printf("mutex_lock init error!\n");
		}
    if (stack_size > 0) {
	   s = pthread_attr_setstacksize(threadpool_p->attr, stack_size);
	   if (s != 0)
		   printf("pthread_attr_setstacksize\n");
    }

	/* create pthreads for pool */
	for( i = 0 ; i < threadpool_p->thread_count ; i++)
	{
		threadpool_p->pthread_ids[i] = pthread_create(&(threadpool_p->threads[i]),threadpool_p->attr,&threadpool_thread,(void *)threadpool_p);
		 printf("MAIN Creating pthread  %lu.\n",threadpool_p->threads[i]);
		if(threadpool_p->pthread_ids[i] !=0)
			printf("Creating pthread %d error!\n",i);
	}
	return threadpool_p;
}
/*
int main(int argc,char **args)
{
	int ret = 0 ,x = 0 ;
	threadpool_t * myPool = NULL;
	addMutex = calloc(1,sizeof(pthread_mutex_t));
	printf("program start!\n");
	pthread_key_create (&key, NULL);
	myPool = createThreadPool(10,100);
	if(myPool)
		printf("createThreadPool suc! \n");
	else
		printf("createThreadPool failed! \n");
	//time_t start=clock();
	for( x = 0 ; ; x++ )
	{
		ret = threadpool_add(myPool ,&hello_pool,(void *)myPool,x);
		if(ret == -1){
			printf("threadpool_add hello pool failed! \n");
			break;
		}
	}
	printf("[MAIN]add tasks %d! \n",x);
	//time_t end=clock();
	//double cost=(end-start)/CLOCKS_PER_SEC;
	sleep(20);
	//printf("cost %lf sec\n",cost);
	ret = shutdownPool(myPool);
	printf("shutdown pool %d.\n",ret);
	waitThreadJoin(myPool);
	destroyThreadPool( myPool);
	pthread_key_delete(key);
	printf("program finished!\n");

	return 0;
}
*/
