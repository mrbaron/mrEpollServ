/*
 * poll.c
 *
 *  Created on: May 30, 2016
 *      Author: mrbaron
 */
#include <sys/epoll.h>

#define MAX_EVENTS 100
#define CALLBACK(x) void (*x) (poll_event_t *, poll_event_element_t *, struct epoll_event)

#define ACCEPT_CB 0x01
#define CONNECT_CB 0x02

typedef struct poll_event_element poll_event_element_t;
typedef struct poll_event poll_event_t;
/**
 * @struct poll_event_element "poll.h"
 * @brief a poll event element containing callbacks, user data and flags
 */
struct poll_event_element_t
{
	int fd;
	CALLBACK(write_callback);
	CALLBACK(read_callback);
	CALLBACK(close_callback);
	CALLBACK(accept_callback);
	CALLBACK(connect_callback);
	void * data;
	uint32_t events;
	uint32_t cur_event;
	uint8_t cb_flags ;
};
#define poll_event_element_s sizeof(poll_event_element_t)

struct poll_event
{
	int (*timeout_callback)(poll_event_t *);
	size_t timeout;
	int epoll_fd;
	void * data;
};
#define poll_event_s sizeof(poll_event_t)
/*
 * alloc a new poll event element
 *
 */
poll_event_element_t * poll_event_element_new(int fd, uint32_t event)
{

}






