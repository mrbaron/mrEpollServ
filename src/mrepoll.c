/*
 * mrepoll.c
 *
 *  Created on: May 30, 2016
 *      Author: mrbaron
 */
#include "mrepoll.h"
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
int sock,conn_sock;
#ifndef MAX_EVENTS
#define MAX_EVENTS 100
#endif
#ifndef MAX_CONN
#define MAX_CONN 1024
#endif
threadpool_t * globalPool;
extern pthread_key_t key;
extern pthread_mutex_t * addMutex;
pthread_mutex_t * writeMutex;

void exit_hook(int number)
{
	int ret ;
	close(sock);
	ret = shutdownPool(globalPool);
	printf("shutdown pool %d.\n",ret);
	waitThreadJoin(globalPool);
	destroyThreadPool( globalPool);
	pthread_key_delete(key);
	printf("program finished!\n");
	printf(">> [%d]will shutdown...[%d]\n", getpid(),number);
	exit(0);
}

void shutdown_epoll_serv(mrepoll_server_t * ms)
{
	if(ms !=NULL)
	{
		if(ms->events !=NULL)
			free(ms->events);
		free(ms);
	}
}

int do_use_fd(int fd)
{
	write(fd,"hello\n",10);
	close(fd);
}

void close_conn (mrepoll_server_t * ms,int fd)
{
    printf("in close_cb");
    close(fd);
    epoll_ctl(ms->efd, EPOLL_CTL_DEL, fd, NULL);
}

int read_conn(int conn_sock ,struct epoll_event *ev ,mrepoll_server_t * ms )
{
   pthread_mutex_lock(addMutex);
   unsigned char *  buf = calloc(1,1024);
   int nread = 0,efd = ms->efd ;
   uint32_t flags = EPOLLOUT | EPOLLRDHUP | EPOLLHUP;
   struct epoll_event eva;
   memset(&eva, 0, sizeof(struct epoll_event));
   eva.data.fd = conn_sock;
   pthread_mutex_unlock(addMutex);
   eva.events = flags;
   if(eva.data.fd<0)
	   return -1;
   fcntl(eva.data.fd, F_SETFL, O_NONBLOCK);
   memset(buf,'\0',sizeof(1024));
   nread = read(eva.data.fd,buf,1024);
   if(nread > 0 )
   {
	   //printf("[READ] READ %s.\n",buf);
   }
   epoll_ctl(efd, EPOLL_CTL_MOD,  eva.data.fd , &eva);
   free(buf);
   // set function callbacks
}

int write_conn(int conn_sock ,struct epoll_event *ev ,mrepoll_server_t * ms )
{
	pthread_mutex_lock(writeMutex);
   int efd = ms->efd,conn_fd =conn_sock;
   pthread_mutex_unlock(writeMutex);
   if(conn_fd<0)
	   return -1;
   fcntl(conn_fd, F_SETFL, O_NONBLOCK);
   write(conn_fd,"Greeting,my friend!\n",20);
   uint32_t flags = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
   struct epoll_event eva;
   memset(&eva, 0, sizeof(struct epoll_event));
   eva.data.fd = conn_fd;
   eva.events = flags;
   epoll_ctl(efd, EPOLL_CTL_MOD, conn_sock, &eva);
   // set function callbacks
}

int accept_conn(int listen_sock ,struct epoll_event *ev ,mrepoll_server_t * ms )
{
   int conn_sock ,i = 0 ;
   struct sockaddr local;
   socklen_t addrlen;
   conn_sock = accept(listen_sock,(struct sockaddr *) &local, &addrlen);
   if (conn_sock == -1) {
	   perror("accept");
	   return -1;
   }
   fcntl(conn_sock, F_SETFL, O_NONBLOCK);

   fprintf(stderr, "got the socket %d\n", conn_sock);
   // set flags to check
   uint32_t flags = EPOLLIN | EPOLLRDHUP | EPOLLHUP;

   struct epoll_event eva;
   memset(&eva, 0, sizeof(struct epoll_event));
   eva.data.fd = conn_sock;
   eva.events = flags;
   if(ms->conn_count >=1024)
	   return -1;

   for( i = 0 ;i < MAX_CONN  ; i++ )
   {
	   if(ms->accept_conns[i].status == 0)
	   {
		   ms->accept_conns[i].fd = conn_sock;
		   ms->accept_conns[i].status = 1;
		   ms->conn_count++;

		   break;
	   }
   }
   // add file descriptor to poll event
   epoll_ctl(ms->efd, EPOLL_CTL_ADD, conn_sock, &eva);
   // set function callbacks
   return 0;
}

int main()
{
	int nfds = 0 ,i,n,j;
	//SIGPIPE handle by kernel
	struct sigaction sa;
	sa.sa_handler=SIG_IGN;
	sigaction(SIGPIPE,&sa,0);
	signal(SIGKILL,exit_hook);
	signal(SIGINT, exit_hook);
	signal(SIGQUIT, exit_hook);
	signal(SIGTERM, exit_hook);
	signal(SIGHUP, exit_hook);

	int ret = 0 ,x = 0 ;
	threadpool_t * myPool = NULL;
	addMutex = calloc(1,sizeof(pthread_mutex_t));
	writeMutex = calloc(1,sizeof(pthread_mutex_t));
	pthread_mutex_init(addMutex,NULL);
	pthread_mutex_init(writeMutex,NULL);
	printf("program start!\n");
	pthread_key_create (&key, NULL);
	myPool = createThreadPool(10,100);
	globalPool = myPool;
	if(myPool)
		printf("createThreadPool suc! \n");
	else
		printf("createThreadPool failed! \n");

	/**
	 * create socket , bind , listen
	 */
	sock = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in srv_addr,in_addr;
	socklen_t slen;
	memset(&srv_addr,0,sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htons(INADDR_ANY);
	srv_addr.sin_port = htons(8080);
	if(bind(sock,(struct sockaddr *)&srv_addr ,sizeof(srv_addr))< 0)
	{
		perror(errno);
		return -1;
	}
	listen(sock,1000);
	fcntl(sock,F_SETFL,O_NONBLOCK);
	mrepoll_server_t * ms = calloc(1,sizeof(mrepoll_server_t));
	poll_event_element_t * evelements = calloc(1,sizeof(poll_event_element_t));
	evelements->fd = sock;
	evelements->events = EPOLLIN;
	evelements->accept_callback = accept_conn;
	evelements->write_callback = write_conn;
	evelements->read_callback = read_conn;
	evelements->close_callback = close_conn;
	ms->accept_conns = calloc(MAX_CONN,sizeof(accept_conn_t));
	ms->conn_count = 0 ;
	memset(ms->accept_conns,'\0',sizeof(accept_conn_t)*MAX_CONN);
	// enable accept callback
	evelements->cb_flags |= ACCEPT_CB;
	ms->events = calloc(MAX_EVENTS,sizeof(struct epoll_event));
	ms->ev.events = EPOLLIN;
	ms->ev.data.fd = sock;
	ms->efd = epoll_create(MAX_EVENTS);
	if(ms->efd == -1)
	{
		perror("epoll_create!");
		exit(EXIT_FAILURE);
	}
	if( epoll_ctl(ms->efd, EPOLL_CTL_ADD,sock, &ms->ev) == -1){
		perror("epoll_ctl: sock");
	}
	for(;;){
		printf("wait..\n");
		nfds = epoll_wait(ms->efd,ms->events,MAX_EVENTS,-1);
		printf("returned..nfds %d\n",nfds);
		if(nfds == -1){
			printf("epoll_wait error %d.\n",errno);
			exit(EXIT_FAILURE);
		}
		if(nfds == 0){
			printf("waittimeout..continue.\n");
			continue;
		}

		for( i = 0; i< nfds;  ++i )
		{
			//printf("nfds = %d,data.fd = %d ,sock= %d,conn_count= %d\n",nfds,ms->events[i].data.fd,sock,ms->conn_count);

			if(ms->events[i].data.fd == sock)
			{
				printf("started processing for event id(%d) and sock(%d)", i, ms->events[i].data.fd);
				// when data avaliable for read or urgent flag is set
				if ((ms->events[i].events & EPOLLIN) || (ms->events[i].events & EPOLLPRI))
				{
					if (ms->events[i].events & EPOLLIN)
					{
						//printf("found EPOLLIN for event id(%d) and sock(%d)\n", i, ms->events[i].data.fd);
						evelements->cur_event &= EPOLLIN;
					}
					else
					{
						//printf("found EPOLLPRI for event id(%d) and sock(%d)\n", i, ms->events[i].data.fd);

						evelements->cur_event &= EPOLLPRI;
					}
					/// connect or accept callbacks also go through EPOLLIN
					/// accept callback if flag set
					if ((evelements->cb_flags & ACCEPT_CB) && (evelements->accept_callback))
						evelements->accept_callback(sock, &ms->ev, ms);
					/// connect callback if flag set
					//if ((evelements->cb_flags & CONNECT_CB) && (evelements->connect_callback))
					//	evelements->connect_callback(poll_event, evelements, ms->events[i]);
					/// read callback in any case
					//if (evelements->read_callback)
					//	evelements->read_callback(poll_event, evelements, ms->events[i]);
				}

				// when write possible



			}else
			{
				//printf("WARNING: check accept_conns for event id(%d) and sock(%d)\n", i, ms->events[i].data.fd);
				for(j = 0 ; j < MAX_CONN ; j++)
				{
					if(ms->events[i].data.fd == ms->accept_conns[j].fd && ms->accept_conns[j].status ==1)
					{
						if(ms->events[i].events & EPOLLIN)
						{
							/// read callback in any case
							if (evelements->read_callback)
								//evelements->read_callback(ms->events[i].data.fd ,&ms->ev ,ms);
							/*
							 * 如果要用线程池处理，就用下面的，否则用上面的，线程池主要考虑的是io耗时的问题（就是说下面的函数执行要多久），如果IO耗时较大则下面的会占优势 */
							{
								ret = threadpool_add(myPool ,&read_conn,ms->events[i].data.fd ,&ms->ev ,ms);
								if(ret == -1){
									printf("threadpool_add hello pool failed! \n");
									break;
								}
							}

						}

						if (ms->events[i].events & EPOLLOUT)
						{
							//printf("found EPOLLOUT for event id(%d) and sock(%d)", i, ms->events[i].data.fd);
							evelements->cur_event &= EPOLLOUT;
							if (evelements->write_callback)
								evelements->write_callback( ms->events[i].data.fd ,&ms->ev ,ms);
							/*
							 * 如果要用线程池处理，就用下面的，否则用上面的，线程池主要考虑的是io耗时的问题（就是说下面的函数执行要多久），如果IO耗时较大则下面的会占优势 */
							{
								ret = threadpool_add(myPool ,&write_conn,ms->events[i].data.fd ,&ms->ev ,ms);
								if(ret == -1){
									printf("threadpool_add hello pool failed! \n");
									break;
								}
							}

						}

						// shutdown or error
						if ( (ms->events[i].events & EPOLLRDHUP) || (ms->events[i].events & EPOLLERR) || (ms->events[i].events & EPOLLHUP))
						{
							if (ms->events[i].events & EPOLLRDHUP)
							{
								//printf("found EPOLLRDHUP for event id(%d) and sock(%d)", i, ms->events[i].data.fd);
								evelements->cur_event &= EPOLLRDHUP;
								ms->accept_conns[j].status = 0 ;
								ms->conn_count --;
							}
							else
							{
								//printf("found EPOLLERR for event id(%d) and sock(%d)", i, ms->events[i].data.fd);
								evelements->cur_event &= EPOLLERR;
								ms->accept_conns[j].status = 0 ;
								ms->conn_count --;
							}
							if (evelements->close_callback)
							{
								evelements->close_callback(ms,ms->events[i].data.fd);
								if(ms->accept_conns[j].status !=0)
									ms->conn_count --;
								ms->accept_conns[j].status =0;
							}
						}
						break;
					}
				}
			}

		}
	}
	ret = shutdownPool(myPool);
	printf("shutdown pool %d.\n",ret);
	waitThreadJoin(myPool);
	destroyThreadPool( myPool);
	pthread_key_delete(key);
	printf("program finished!\n");
	shutdown_epoll_serv(ms);
}

