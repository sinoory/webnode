
#include "zebra.h"


typedef struct {
	u_int32_t alloc ;

	u_int32_t total ;
	u_int32_t used ; 

	struct stream * head ;
	struct stream * tail ;
} wjmem_t ;

extern wjmem  wjmem_new(void) 
{
	return (wjmem)XCALLOC( MTYPE_TMP, sizeof(wjmem_t));
}
extern void   wjmem_destroy(wjmem wjm)
{
	struct stream * s , *next ;
	for( s= ((wjmem_t*)wjm)->head ; s && (next=s->next,1) ; s = next ) 
	{
		stream_free(s);
	}
	XFREE(MTYPE_TMP , wjm) ;
}

extern void * wjmem_malloc(wjmem wjm, u_int32_t size)
{
	wjmem_t * wm ;
	struct stream * s ;
	u_int32_t rsize ;
	void * addr = NULL ;

	rsize = ( (size) +sizeof(long)- 1UL) & ( ~(sizeof(long)-1UL) ) ;
	
	if( NULL ==(wm=wjm) || size <= 0 || size > WJMEM_BUFF_SIZE )
		return NULL ;

	// head 
	for( s=wm->head ; s ; s=s->next )
	if( STREAM_WRITEABLE(s)>=rsize ) 
	{
		addr = STREAM_DATA(s) + stream_get_endp(s) ;
		stream_forward_endp(s, rsize) ;

		wm->used += rsize ;
		return addr ;
	}

	// alloc new one 
	s = stream_new(WJMEM_BUFF_SIZE) ;
	if( NULL == s )
		return NULL ;

	addr = STREAM_DATA(s) ;
	stream_forward_endp(s, rsize) ;
	wm->alloc ++ ;
	wm->total += WJMEM_BUFF_SIZE ;
	wm->used  += rsize ;

	if( rsize >= (WJMEM_BUFF_SIZE - (WJMEM_BUFF_SIZE>>2)) )
	{
		s->next = NULL ;
		wm->tail = s ;
		if( !wm->head )
		    wm->head = s ;
	}
	else
	{
		s->next = wm->head ;
		wm->head = s ;
		if( !wm->tail )
		    wm->tail = s ;
	}

	return addr ;
}
extern char * wjmem_strdup(wjmem wjm, char * string) 
{
	wjmem_t * wm ; 
	u_int32_t len ;
	char    * res ;

	if( NULL ==(wm = wjm) || NULL == string || '\0' == *string )
		return NULL ;

	len = strnlen(string, WJMEM_BUFF_SIZE) ;
	if( len >= WJMEM_BUFF_SIZE ) 
	{
		return NULL ; // too large
	}

	res = wjmem_malloc(wjm, len+1) ;
	if( NULL != res )
	{
		memcpy(res, string, len) ;
		res[len] = 0 ;
	}
	return res ;
}



