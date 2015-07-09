#ifndef APX_TYPE_H
#define APX_TYPE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <sys/wait.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <semaphore.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

#define BIT(nr)			(1UL << (nr))





#endif

