/*
 * mrepoll.h
 *
 *  Created on: May 30, 2016
 *      Author: mrbaron
 */

#ifndef SRC_MREPOLL_H_
#define SRC_MREPOLL_H_

#include <sys/epoll.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "poll.h"
#endif /* SRC_MREPOLL_H_ */


#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#define MAX_THREADS 64
#define MAX_QUEUE 10240

typedef struct{
	void (*function) (int ,struct epoll_event * ,mrepoll_server_t  *);
	int arg;
	struct epoll_event * arg2;
	mrepoll_server_t * arg3;
}threadpool_task_t;

typedef struct {
	pthread_t * threads ;
	int *pthread_ids ;
	int *saveflag ;
	int thread_count ;
	int queue_count;
	int head ;
	int tail ;
	int count ;
	int shutdown ;
	int start;
	pthread_cond_t notify;
	threadpool_task_t * queue;
	pthread_attr_t *attr;
	pthread_mutex_t mutex_lock;
}threadpool_t;
//typedef struct threadpool_t threadpool_t;

typedef enum{
	threadpool_normal = 0,
	threadpool_invalid = -1,
	threadpool_lock_failure = -2,
	threadpool_queue_full = -3,
	threadpool_shutdown = -4,
	threadpool_joined = -5
}ptheadpool_error_t;

/**
 * @function createThreadPool
 * @brief Creates a threadpool_t object.
 * @param maxPthread Number of worker threads.
 * @param maxQueue   Size of the queue.
 * @return a newly created thread pool or NULL
 */
threadpool_t * createThreadPool(int maxPthread,int maxQueue);

/**
 * @function destroyThreadPool
 * @brief destroy a threadpool_t object.
 * @param threadpool_t * pool ,which to destroy.
 * @return a newly created thread pool or NULL
 */
int destroyThreadPool(threadpool_t * pool);

/**
 * @function threadpool_add
 * @brief add task to threadpool
 * @param threadpool_t * pool ,add to it
 * @param void (*function) (void*) , a function add to task
 * @param void * arg , a argument to function.
 * @return a newly created thread pool or NULL
 */
int threadpool_add(threadpool_t *pool ,void (*function) (void*),int arg,struct epoll_event * arg2,mrepoll_server_t * arg3);
#endif
