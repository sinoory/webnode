/*-----------------------------------------------------------
*      Copyright (c)  AppexNetworks, All Rights Reserved.
*
*FileName:     sha1sum.h 
*
*Description:  sha1sum.h
* 
*History: 
*      Author              Date        	Modification 
*  ----------      ----------  	----------
* 	xyfeng   		2015-7-2     	Initial Draft 
* 
*------------------------------------------------------------*/
#ifndef _SHA1SUM_H_
#define _SHA1SUM_H_
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
        
/*---Include Local.h File---*/
        
#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
#define SHA1_DIGEST_SIZE 20
        
/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
typedef unsigned int uint32_t;

/* Structure to save state of computation between the single steps.  */
struct sha1_ctx
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
	uint32_t E;

	uint32_t total[2];
	uint32_t buflen;
	uint32_t buffer[32];
};
        
/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
void sha1_init_ctx (struct sha1_ctx *ctx);
void sha1_process_bytes (const void *buffer, size_t len, struct sha1_ctx *ctx);
void *sha1_finish_ctx (struct sha1_ctx *ctx, void *resbuf);        

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _SHA1SUM_H_ */       
         
        

