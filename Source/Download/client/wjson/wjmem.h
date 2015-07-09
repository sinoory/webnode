#ifndef __WJMEM_H__
#define __WJMEM_H__ 1 

#define WJMEM_BUFF_SIZE (4096U)

typedef void * wjmem ;

extern wjmem  wjmem_new(void) ;
extern void   wjmem_destroy(wjmem wjm) ;

extern void * wjmem_malloc(wjmem wjm, u_int32_t size);
extern char * wjmem_strdup(wjmem wjm, char * string) ;

#endif

