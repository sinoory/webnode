#define _GNU_SOURCE /* for strnlen */
#include "zebra.h"
#include "listhead.h"
#include "proto_base.h"

typedef  struct  __wjson_node__ {
	struct list_head list ;
	struct __wjson_node__ * parent ;
	wjmem                   memory ;
	wjson_type_e type ;
	char       * name ;
	char         data[] ;
} wjson_t ;
wjson_t * __wjson_new(wjson_type_e type)
{
	wjson_t * wj ;
	wjmem     mm ;

	mm = wjmem_new() ;
	if( NULL == mm )
		return NULL ;

	wj = wjmem_malloc(mm, sizeof(wjson_t)+sizeof(struct list_head));
	if( NULL == wj )
	{
		wjmem_destroy(mm);
		return NULL ;
	}

	wj->type = type ;
	wj->name = NULL ;
	wj->parent = NULL ;
	wj->memory = mm ;
	INIT_LIST_HEAD(&wj->list) ;
	INIT_LIST_HEAD( (struct list_head*)wj->data );
	return wj ;
}
void  wjson_free(wjson wm)
{
	wjson_t * wj = (wjson_t*)wm ;
	assert( wj && wj->memory && !wj->parent ) ;
	wjmem_destroy(wj->memory) ;
}
wjson wjson_new_array(void)
{
	return (wjson)__wjson_new(WJSON_TYPE_ARRAY) ;
}
wjson wjson_new_object(void)
{
	return (wjson)__wjson_new(WJSON_TYPE_OBJECT) ;
}
wjson_t * __add_item(wjson_t *wj, wjson_type_e type, const char * name, void * data, u_int32_t datalen)
{
	wjson_t * wjpos ;

	assert(!( !wj || !data || datalen <= 0 ));

	if( WJSON_TYPE_OBJECT == wj->type )
	{
		if( NULL == name || '\0' == *name )
			return NULL ;

		list_for_each_entry(wjpos, (struct list_head*)wj->data , list)
		{
			if( strncmp(name, wjpos->name, WJSON_NAME_SIZE) == 0 )
				return NULL ; // exsit 
		}
	}
	else if( WJSON_TYPE_ARRAY ==  wj->type )
	{
		name = NULL ; // no need 
	}
	else
	{
		return NULL ;// no support 
	}

	wjpos = wjmem_malloc(wj->memory, sizeof(wjson_t) + datalen ) ;
	if( NULL != wjpos )
	{
		wjpos->parent = wj ;
		wjpos->memory = wj->memory ;
		wjpos->type   = type ;
		wjpos->name   = wjmem_strdup(wj->memory, (char*)name) ;

		memcpy(wjpos->data, data, datalen) ;
		
		INIT_LIST_HEAD(& wjpos->list) ;
		list_add_tail(&wjpos->list, (struct list_head*)wj->data) ;
	}

	return wjpos ;
}
wjson wjson_add_int    (wjson wj, const char * name, int value)
{
	return (wjson)__add_item(wj, WJSON_TYPE_INT , name, 
		&value , sizeof(value)) ;
}
wjson_type_e wjson_get_type(wjson wj)
{
	if( NULL == wj )
		return WJSON_TYPE_ERROR ;
	return ((wjson_t*)wj)->type ;
}

char* wjson_get_name(wjson wj  )
{
	if( NULL == wj )
		return NULL;
	return ((wjson_t*)wj)->name;
}

int wjson_get_int(wjson wj, int def )
{
	if( NULL == wj )
		return def ;
	return *(int*)( ((wjson_t*)wj)->data );
}

char* wjson_get_string(wjson wj, char* def )
{
	if( NULL == wj )
		return def ;
	return ((wjson_t*)wj)->data;
}

wjson wjson_add_string (wjson wj, const char * name, char * string)
{
	return (wjson)__add_item(wj, WJSON_TYPE_STRING, name, 
		string, strnlen(string, WJSON_BUFF_SIZE) + 1);
}
wjson wjson_add_string_length (wjson wj, const char * name, char * string, u_int32_t length)
{
	wjson_t * item = __add_item(wj, WJSON_TYPE_STRING, name, 
		string, length+1 ); // +1 for '\0'
	if( NULL != item )
	{
		item->data[length] = 0 ; // '\0' for STRING
	}
	return item ;
}
wjson wjson_add_object (wjson wj, const char * name )
{
	struct list_head head ;
	wjson_t * n ; 

	n = __add_item(wj, WJSON_TYPE_OBJECT, name ,
		&head, sizeof(head)) ;
	if( NULL != n )
		INIT_LIST_HEAD((struct list_head*)n->data) ;

	return n ;
}
wjson wjson_add_array  (wjson wj, const char * name )
{
	struct list_head head ;
	wjson_t * n ; 

	n = __add_item(wj, WJSON_TYPE_ARRAY, name ,
		&head, sizeof(head)) ;
	if( NULL != n )
		INIT_LIST_HEAD((struct list_head*)n->data) ;

	return n ;
}

int wjson_for_each( wjson pwj, wjson_iterator iterator, void * arg1, void * arg2 ) 
{
    wjson_t * item ;
    struct list_head * head; 
    wjson_t * wj = (wjson_t*)pwj ;
    int ret = 0;

    assert(wj&&iterator) ;
    assert(wj->type==WJSON_TYPE_OBJECT||wj->type==WJSON_TYPE_ARRAY);

    head = (struct list_head *) wj->data ;
    list_for_each_entry(item, head, list) 
    {
        ret = iterator( item, arg1, arg2 ) ;
        if( 0 != ret ) 
            break ;
    }

    return ret ;
}


wjson wjson_lookup_by_name(wjson pwj, const char * name)
{
    wjson_t * match ;
    wjson_t * wj = (wjson_t*)pwj ;

    if( wj && WJSON_TYPE_OBJECT == wj->type )
    {
        list_for_each_entry(match, (struct list_head*)wj->data, list)
        {
            if( match->name && strcasecmp(match->name, name)== 0 )
                return match ;
        }
    }

    return NULL ;
}
char * wjson_lookup_string_by_name(wjson pwj, const char *name, char * def)
{
    wjson_t * match ;

    match = wjson_lookup_by_name(pwj, name) ;
    if( match && match->type == WJSON_TYPE_STRING)
    {
        return match->data ;
    }
    return def ;
}
int wjson_lookup_number_by_name(wjson pwj, const char *name, int def)
{
    wjson_t * match ;

    match = wjson_lookup_by_name(pwj, name) ;
    if( match && match->type == WJSON_TYPE_INT)
    {
        return *(int*)match->data ;
    }
    return def ;
}


struct buffer * wjson_to_buffer( wjson _wj ) 
{
	struct buffer * res, *sres ;
	wjson_t * wj = _wj ,*swj ;

	if( NULL == wj )
		return NULL ;

	res = buffer_new(WJSON_BUFF_SIZE) ;
	if( NULL == res )
		return NULL ;

	if( wj->name )
	{
		buffer_printf(res, "\"%s\":" , wj->name ) ;
	}

	switch( wj->type )
	{
		case WJSON_TYPE_INT :
			buffer_printf(res, "%d", *(int*)wj->data ) ;
			break; 
		case WJSON_TYPE_STRING: 
			buffer_printf(res, "\"%s\"", (char*)wj->data ) ;
			break; 
		case WJSON_TYPE_OBJECT:
		case WJSON_TYPE_ARRAY :
			buffer_putc(res, (wj->type==WJSON_TYPE_ARRAY)?'[':'{');
			list_for_each_entry( swj, (struct list_head*)wj->data, list)
			{
				sres = wjson_to_buffer(swj) ;
				if( NULL == sres )
				{
					// break; 
					exit(0);
				}
				else 
				buffer_join( res, sres );

				if(! list_is_last( &swj->list ,(struct list_head*)wj->data) )
				{
					buffer_putc(res, ',');
				}
			}
			buffer_putc(res, (wj->type==WJSON_TYPE_ARRAY)?']':'}');
			break ;
		default :
			break ;
	}

	return res ;
}

char *wjson_string( wjson _wj )
{
	char *pstr;
	wjson_t * wj = _wj;
	struct buffer * res ;
	
	if( NULL == wj )
	{
		return NULL;
	}

	res = wjson_to_buffer( wj );
	if( NULL == res )
	{
		return NULL;
	}
	
	pstr = buffer_getstr( res );
	buffer_free( res );

	return pstr;
}
typedef enum { 
	TOKEN_INIT = 0 ,
	TOKEN_NUMBER ,
	TOKEN_HEX ,
	TOKEN_STRING ,
	TOKEN_KEYWORD,
	TOKEN_SEPRATOR ,
	TOKEN_ATTRIBUTE, 
	TOKEN_AOPEN ,
	TOKEN_ACLOSE,
	TOKEN_OOPEN ,
	TOKEN_OCLOSE, 
	TOKEN_EOF ,
	TOKEN_MAX ,
} ttype_e ;
#define  DEST_ENTRY(T)  [T] = #T 
static const char * token_string[] = {
	DEST_ENTRY(TOKEN_INIT),
	DEST_ENTRY(TOKEN_NUMBER) ,
	DEST_ENTRY(TOKEN_HEX),
	DEST_ENTRY(TOKEN_STRING ),
	DEST_ENTRY(TOKEN_KEYWORD),
	DEST_ENTRY(TOKEN_SEPRATOR ),
	DEST_ENTRY(TOKEN_ATTRIBUTE),
	DEST_ENTRY(TOKEN_AOPEN) ,
	DEST_ENTRY(TOKEN_ACLOSE),
	DEST_ENTRY(TOKEN_OOPEN ),
	DEST_ENTRY(TOKEN_OCLOSE), 
	DEST_ENTRY(TOKEN_EOF) ,
	DEST_ENTRY(TOKEN_MAX) ,
} ;
typedef struct {
	char * ptr ;
	u_int32_t len ;
	ttype_e type ;
	int     hold ;
} token_t ;
#define TOKEN_READY(t) ((t)->hold)
#define TOKEN_DONE(t)  (t)->hold = 0
#define TOKEN_HOLD(t)  (t)->hold = 1
typedef struct {
	char * data ;
	u_int32_t length ;
	u_int32_t position ;

	token_t last ;
	token_t token ;
} parser_t ;

char get_char( parser_t * p)
{
	if( p->position < p->length )
		return p->data[ p->position ++ ] ;
	return 0 ;
}
char get_char_no_inc( parser_t * p)
{
	if( p->position < p->length )
		return p->data[ p->position ] ;
	return 0 ;
}
char * get_ptr(parser_t *p )
{
	if( p->position < p->length )
		return p->data + p->position ;
	return p->data + p->length  ;
}
#define is_key_char(c) ( ('a'<=(c)&&(c)<='z')||('A'<=(c)&&(c)<='Z')||('0'<=(c)&&(c)<='9')|| \
                          '_'==(c)|| '.'==(c)|| '-' == (c) )
#define is_hex_char(c) ( ('a'<=(c)&&(c)<='z')||('A'<=(c)&&(c)<='Z')||('0'<=(c)&&(c)<='9') )
#define is_space_char(c) (' '==(c)||'\r'==(c)||'\n'==(c)||'\t'==(c))
token_t * get_token( parser_t *p )
{
	char ch , nch, nch2, nch3,nch4;
	token_t * token = &p->token ;
	int       offset= 0 ;

	if( TOKEN_READY(token) )
	{
		TOKEN_DONE(token);
		return token ;
	}
	// save last 
	p->last = p->token ;

	do {
		token->ptr = get_ptr(p) ;
		ch = get_char(p) ;
	} while( is_space_char(ch)  ) ;
	token->type = TOKEN_EOF ; 
	token->hold = 0 ;
	token->len  = 1 ;

	switch( ch )
	{
		case '[' :
			token->type = TOKEN_AOPEN ;
			break;
		case ']' :
			token->type = TOKEN_ACLOSE ;
			break; 
		case '{' :
			token->type = TOKEN_OOPEN ;
			break; 
		case '}' :
			token->type = TOKEN_OCLOSE ;
			break; 
		case ',' :
			token->type = TOKEN_SEPRATOR ;
			break; 
		case ':' :
			token->type = TOKEN_ATTRIBUTE ;
			break; 
		case '\"':
		case '\'':
			token->ptr  = get_ptr(p);
			do {
				nch = get_char(p) ;
				if( '\\' == nch )
				{
					get_char(p) ;
					continue ;
				}
			}while( nch && nch != ch );
			if( nch == ch )
			{
				offset = 1 ;
				token->type = TOKEN_STRING;
				break; 
			}
			break; 
			
		case 'n':
		{/** null */
			nch 	=  get_char(p);
			nch2	=  get_char(p);
			nch3	=  get_char(p);
			if( 'u' == nch && 'l' == nch2 && 'l' == nch3 )
			{
				offset = 4;
				token->ptr	= get_ptr(p) - 4;
				token->type 	= TOKEN_STRING;
			}
			else
			{
				p->position -= 4;
			}
		}
		break;
		
		case 't':
		{/** true */
			nch 	=  get_char(p);
			nch2	=  get_char(p);
			nch3	=  get_char(p);
			if( 'r' == nch && 'u' == nch2 && 'e' == nch3 )
			{
				offset = 4;
				token->ptr	= get_ptr(p) - 4;
				token->type 	= TOKEN_STRING;
			}
			else
			{
				p->position -= 4;
			}
		}
		break;
		
		case 'f':
		{/** false */
			nch 	=  get_char(p);
			nch2	=  get_char(p);
			nch3	=  get_char(p);
			nch4	=  get_char(p);
			if( 'a' == nch && 'l' == nch2 && 's' == nch3 && 'e' == nch4 )
			{
				offset = 5;
				token->ptr	= get_ptr(p) - 5;
				token->type 	= TOKEN_STRING;
			}
			else
			{
				p->position -= 5;
			}
		}
		break;

		default :
			nch = get_char_no_inc(p);
			if( '0' == ch && ('x' == nch || 'X' == nch ) ) // HEX NUMBER 
			{
				get_char(p); // skip x
				do {
					nch = get_char(p) ;
				} while( is_hex_char(nch) ) ;
				p->position -- ;
				token->type = TOKEN_HEX ;
			}
			else if( '0' <= ch && ch <= '9' )   // NORMAL NUMBER 
			{
				do {
					nch = get_char(p) ;
				}while( ( '0' <= nch && nch  <= '9' ) || ( '.' == nch ) ) ;
				p->position --;
				token->type = TOKEN_NUMBER ;
			}
			else if ( is_key_char(ch) ) // KEYWORD NAME 
			{
				do {
					nch = get_char(p);
				}while( is_key_char(nch) );
				p->position -- ;
				token->type = TOKEN_KEYWORD ;
			}
			break; 
	}
	token->len = (u_int32_t)(long)(get_ptr(p) - token->ptr ) - offset ;

	return token ;
}

wjson wjson_from_string(char * string, u32 length) 
{
	wjson_t * wj , * root, *it ; 
	parser_t __local_parser, * p  = &__local_parser;
	token_t * token ;
	char name[WJSON_NAME_SIZE+2];
	char * nptr = NULL ;
	u_int32_t value = 0 ;
	u_int32_t canDone =0 ;

	if( NULL == string || length <= 0 )
		return NULL ;

	memset(p, 0, sizeof(parser_t));
	p->data = string ;
	p->length = length ;

	token = get_token(p) ;
	if( TOKEN_AOPEN == token->type )
		wj = wjson_new_array();
	else if(TOKEN_OOPEN==token->type)
		wj = wjson_new_object();
	else
		return NULL ;

	root = wj ; /* save root wjson first */
	while( wj )
	{
		token = get_token(p) ;
#ifdef WJSON_DEBUG
		printf("TYPE %14s, LEN %08x, STRING %.*s\n", 
			token_string[token->type], token->len, token->len, token->ptr) ;
#endif
		if( 1 == canDone && 
		   ((TOKEN_ACLOSE == token->type && WJSON_TYPE_ARRAY == wj->type) ||
		    (TOKEN_OCLOSE == token->type && WJSON_TYPE_OBJECT== wj->type)) )
		{
			wj = wj->parent ;
		}
        else
        {
    		name[0] = 0 ;
    		if( WJSON_TYPE_OBJECT == wj->type )
    		{
    			if( TOKEN_STRING != token->type && TOKEN_KEYWORD != token->type )
    			{
    				wjson_free(root) ;
    				//printf("%s, [%s]\n", token_string[token->type], token->ptr );
    				assert(0);
    				return NULL ;
    			}
    			if( token->len >= WJSON_NAME_SIZE )
    			{
    				// too long 
    				// ignore yet,
    				token->len = WJSON_NAME_SIZE ;
    			}
    			memcpy(name, token->ptr, token->len) ;
    			name[token->len] = 0 ;
    			
    			token = get_token(p);
    			if( TOKEN_ATTRIBUTE != token->type )
    			{
    				wjson_free(root);
    				printf("%s, [%s]\n", token_string[token->type], token->ptr );
    				assert(0);
    				return NULL ;
    			}

    			token = get_token(p) ;
    			#ifdef WJSON_DEBUG 
    			printf("TYPE %14s, LEN %08x, STRING %.*s\n", 
    				token_string[token->type], token->len, token->len, token->ptr) ;
    			#endif
    		}

    		switch(token->type)
    		{
    			case TOKEN_AOPEN :
    				it = wj = wjson_add_array(wj, name) ;
    				break; 
    			case TOKEN_OOPEN :
    				it = wj = wjson_add_object(wj, name);
    				break; 
    			case TOKEN_STRING :
    				it = wjson_add_string_length(wj, name, token->ptr, token->len);
    				break; 
    			case TOKEN_NUMBER :
    				{
    					if( token->len > 1 && *token->ptr == '0' )
    						value = strtol(token->ptr, &nptr, 8 ) ;
    					else 
    						value = atoi(token->ptr) ;
    					it = wjson_add_int(wj, name, value );
    				}
    				break; 
    			case TOKEN_HEX :
    				{
    					value = strtol(token->ptr, &nptr, 16) ;
    					it    = wjson_add_int(wj, name, value);
    				}
    				break ;
    			default : 
    				it = NULL ;
    				break; 
    		}

    		if( NULL == it )
    		{
                //printf("%s, [%s]\n", token_string[token->type], token->ptr );
    			wjson_free(root);
    			assert(0); 
    			return NULL ;
    		}
        }

        // skip seprator 
		token  = get_token(p);
		canDone= 0 ;
		if( token->type != TOKEN_SEPRATOR )
		{
			TOKEN_HOLD(token);
			canDone = 1 ;
		}
	}
	
	return (wjson)root ;
}


