#include <stdio.h>
#include <sys/time.h>

/* Return a character string representing the current date and time.  */
char* get_timestamp( void )
{
	static char timestamp[64] = {0};
	struct timeval tv;

	gettimeofday( &tv, NULL );
	snprintf( timestamp, sizeof( timestamp ), "%ld.%ld", tv.tv_sec, tv.tv_usec );

	return timestamp;
}

#ifdef TIMESTAMP_DEBUG
#include <stdlib.h>
int main (int argc, char* argv[])
{
    int k = 0;
    int cnt = 1;
    char buf[64] = { 0 };
    size_t len = sizeof( buf  );

    if( argc > 1 )
    {
        cnt = atoi( argv[1] );
    }
    /* Get the current timestamp.  */
    for( k = 0; k < cnt; k++ )
    {
       printf( "%s\n", get_timestamp() );
    }

    return 0;
}
#endif

