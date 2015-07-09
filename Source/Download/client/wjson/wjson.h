#ifndef __WJSON_H__ 
#define __WJSON_H__ 1 

#define WJSON_BUFF_SIZE  (4096U)
#define WJSON_NAME_SIZE  (64U) 

typedef enum {
	WJSON_TYPE_INT ,
    WJSON_TYPE_STRING,
    WJSON_TYPE_OBJECT,
    WJSON_TYPE_ARRAY ,

	WJSON_TYPE_MAX,
} wjson_type_e ;
#define WJSON_TYPE_ERROR WJSON_TYPE_MAX 

typedef void * wjson ;
typedef int (*wjson_iterator)(wjson wj, void * arg1, void * arg2 ) ;

wjson wjson_new_array(void);
wjson wjson_new_object(void);
void  wjson_free(wjson wj);

wjson wjson_add_int    (wjson wj, const char * name, int value);
wjson wjson_add_string (wjson wj, const char * name, char * string);
wjson wjson_add_object (wjson wj, const char * name ); 
wjson wjson_add_array  (wjson wj, const char * name );
int   wjson_for_each( wjson pwj, wjson_iterator iterator, void * arg1, void * arg2 ) ;

wjson_type_e wjson_get_type(wjson wj);
wjson wjson_lookup_by_name(wjson pwj, const char * name);

char * wjson_lookup_string_by_name(wjson pwj, const char *name, char * def);
int    wjson_lookup_number_by_name(wjson pwj, const char *name, int def);


struct buffer * wjson_to_buffer( wjson _wj ) ;
wjson wjson_from_string(char * string, u32 length);




#endif
