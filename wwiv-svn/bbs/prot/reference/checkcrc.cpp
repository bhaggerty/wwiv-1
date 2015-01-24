#ifndef	lint
static const char rcsid[] = "$Id: checkcrc.cpp 6 2004-09-18 20:02:00Z rushfan $" ;
#endif

#include <stdio.h>
#include <sys/types.h>

#include "crctab.h"


static	u_char	msg0[] = {
	  0x2a, 0x18, 0x43, 0x0a, 0x00, 0x00, 0x00, 0x00, 
	  0xbc, 0xef, 0x92, 0x8c,} ;
static	u_char	msg0c[] = {
	  0x2a, 0x18, 0x43, 0x0a, 0x00, 0x00, 0x00, 0x00,
	  0xbc, 0xef, 0x92, 0x8c, 0x0b, 0xdf,} ;
static	u_char	msg1[] = {
	  0x0a, 0x23, 0x69, 0x6e, 
	  0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 0x3c, 0x73, 
	  0x79, 0x73, 0x2f, 0x74, 0x65, 0x72, 0x6d, 0x69, 
	  0x6f, 0x73, 0x2e, 0x68, 0x3e, 0x0a, 0x23, 0x69, 
	  0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 0x22, 
	  0x78, 0x6d, 0x6f, 0x64, 0x65, 0x6d, 0x2e, 0x68, 
	  0x22, 0x0a, 0x0a, 0x0a, 0x69, 0x6e, 0x74, 0x0a, 
	  0x73, 0x65, 0x6e, 0x64, 0x43, 0x61, 0x6e, 0x63, 
	  0x65, 0x6c, 0x28, 0x29, 0x0a, 0x7b, 0x0a, 0x09, 
	  0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x73, 
	  0x65, 0x6e, 0x64, 0x46, 0x6c, 0x75, 0x73, 0x68, 
	  0x28, 0x43, 0x41, 0x4e, 0x29, 0x20, 0x7c, 0x7c, 
	  0x20, 0x73, 0x65, 0x6e, 0x64, 0x46, 0x6c, 0x75, 
	  0x73, 0x68, 0x28, 0x43, 0x41, 0x4e, 0x29, 0x20, 
	  0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x0a, 0x09, 0x2f, 
	  0x2a, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20, 0x6f, 
	  0x6e, 0x65, 0x20, 0x63, 0x68, 0x61, 0x72, 0x61, 
	  0x63, 0x74, 0x65, 0x72, 0x2c, 0x20, 0x72, 0x65, 
	  0x74, 0x75, 0x72, 0x6e, 0x20, 0x6e, 0x6f, 0x6e, 
	  0x7a, 0x65, 0x72, 0x6f, 0x20, 0x6f, 0x6e, 0x20, 
	  0x65, 0x72, 0x72, 0x6f, 0x72, 0x20, 0x2a, 0x2f, 
	  0x0a, 0x69, 0x6e, 0x74, 0x0a, 0x73, 0x65, 0x6e, 
	  0x64, 0x46, 0x6c, 0x75, 0x73, 0x68, 0x28, 0x63, 
	  0x29, 0x0a, 0x09, 0x63, 0x68, 0x61, 0x72, 0x09, 
	  0x63, 0x20, 0x3b, 0x0a, 0x7b, 0x0a, 0x09, 0x2f, 
	  0x2a, 0x20, 0x66, 0x69, 0x72, 0x73, 0x74, 0x2c, 
	  0x20, 0x66, 0x6c, 0x75, 0x73, 0x68, 0x20, 0x69, 
	  0x6e, 0x70, 0x75, 0x74, 0x20, 0x70, 0x6f, 0x72, 
	  0x74, 0x20, 0x2a, 0x2f, 0x0a, 0x09, 0x2f, 0x2a, 
	  0x20, 0x54, 0x4f, 0x44, 0x4f, 0x3a, 0x20, 0x63, 
	  0x61, 0x6c, 0x6c, 0x65, 0x72, 0x20, 0x70, 0x72, 
	  0x6f, 0x76, 0x69, 0x64, 0x65, 0x20, 0x61, 0x20, 
	  0x77, 0x61, 0x79, 0x20, 0x18, 0x69, 0x4c, 0x5b, 
	  0x62, 0x0f,
	  0x2a, 0x18, 0x43, 0x0a, 0x00, 0x00, 
	  0x00, 0x00, 0xbc, 0xef, 0x92, 0x8c, 0x0a, 0x23, 
	  0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 
	  0x3c, 0x73, 0x79, 0x73, 0x2f, 0x74, 0x65, 0x72, 
	  0x6d, 0x69, 0x6f, 0x73, 0x2e, 0x68, 0x3e, 0x0a, 
	  0x23, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 
	  0x20, 0x22, 0x78, 0x6d, 0x6f, 0x64, 0x65, 0x6d, 
	  0x2e, 0x68, 0x22, 0x0a, 0x0a, 0x0a, 0x69, 0x6e, 
	  0x74, 0x0a, 0x73, 0x65, 0x6e, 0x64, 0x43, 0x61, 
	  0x6e, 0x63, 0x65, 0x6c, 0x28, 0x29, 0x0a, 0x7b, 
	  0x0a, 0x09, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 
	  0x20, 0x73, 0x65, 0x6e, 0x64, 0x46, 0x6c, 0x75, 
	  0x73, 0x68, 0x28, 0x43, 0x41, 0x4e, 0x29, 0x20, 
	  0x7c, 0x7c, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x46, 
	  0x6c, 0x75, 0x73, 0x68, 0x28, 0x43, 0x41, 0x4e, 
	  0x29, 0x20, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x0a, 
	  0x09, 0x2f, 0x2a, 0x20, 0x73, 0x65, 0x6e, 0x64, 
	  0x20, 0x6f, 0x6e, 0x65, 0x20, 0x63, 0x68, 0x61, 
	  0x72, 0x61, 0x63, 0x74, 0x65, 0x72, 0x2c, 0x20, 
	  0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x6e, 
	  0x6f, 0x6e, 0x7a, 0x65, 0x72, 0x6f, 0x20, 0x6f, 
	  0x6e, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x20, 
	  0x2a, 0x2f, 0x0a, 0x69, 0x6e, 0x74, 0x0a, 0x73, 
	  0x65, 0x6e, 0x64, 0x46, 0x6c, 0x75, 0x73, 0x68, 
	  0x28, 0x63, 0x29, 0x0a, 0x09, 0x63, 0x68, 0x61, 
	  0x72, 0x09, 0x63, 0x20, 0x3b, 0x0a, 0x7b, 0x0a, 
	  0x09, 0x2f, 0x2a, 0x20, 0x66, 0x69, 0x72, 0x73, 
	  0x74, 0x2c, 0x20, 0x66, 0x6c, 0x75, 0x73, 0x68, 
	  0x20, 0x69, 0x6e, 0x70, 0x75, 0x74, 0x20, 0x70, 
	  0x6f, 0x72, 0x74, 0x20, 0x2a, 0x2f, 0x0a, 0x09, 
	  0x2f, 0x2a, 0x20, 0x54, 0x4f, 0x44, 0x4f, 0x3a, 
	  0x20, 0x63, 0x61, 0x6c, 0x6c, 0x65, 0x72, 0x20, 
	  0x70, 0x72, 0x6f, 0x76, 0x69, 0x64, 0x65, 0x20, 
	  0x61, 0x20, 0x77, 0x61, 0x79, 0x20, 0x18, 0x6b, 
	  0x60, 0x3a, 0x6c, 0xe1, 
	} ;
static	u_char	msg2[] = {1} ;

#ifdef	COMMENT
#define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)
#define updcrc(cp, crc) ( crctab[((crc>>8)^cp) & 255] ^ (crc<<8) )
#endif	/* COMMENT */

static	int	addcrc(int data, int crc) ;


main()
{
	u_short	crc16 ;
	u_long	crc32 ;
	int	i,j ;
	u_long	k ;
	u_char	*msg = msg0 ;
	int	len = sizeof(msg0) ;


	/* crc16 of message */

	crc16 = 0 ;
	crc32 = 0xffffffff ;

	for(i=0; i<len; ++i)
	{
	  crc16 = addcrc(msg[i], crc16) ;
	}

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;

	crc16 = addcrc(0, crc16) ;
	crc16 = addcrc(0, crc16) ;

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;


	/* crc of reception */

	j = crc16 ;
	crc16 = 0 ;
	crc32 = 0xffffffff ;

	for(i=0; i<len; ++i)
	{
	  crc16 = addcrc(msg[i], crc16) ;
	}

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;

	crc16 = addcrc(j>>8, crc16) ;
	crc16 = addcrc(j&0xff, crc16) ;

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;



#ifdef	COMMENT
	crc16 = 0 ;
	crc32 = 0xffffffff ;

	for(i=0; i<len; ++i)
	{
	  crc16 = updcrc(msg[i], crc16) ;
	  crc32 = UPDC32(msg[i], crc32) ;
	  printf("%4.4x\n", crc16) ;
	}

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;
	printf("crc32=%8.8x, ~crc=%8.8x\n", crc32,~crc32) ;


#ifdef	COMMENT
	printf("\n") ;
	printf("%2.2x\n", crc16&0xff) ;
	printf("%2.2x\n", crc16>>8) ;
#endif	/* COMMENT */

	i = crc16 ;
	crc16 = updcrc(i&0xff, crc16) ;
	crc16 = updcrc(i>>8, crc16) ;

	k = ~crc32 ;
	crc32 = updcrc(k&0xff, crc32) ;
	crc32 = updcrc((k>>8)&0xff, crc32) ;
	crc32 = updcrc((k>>16)&0xff, crc32) ;
	crc32 = updcrc((k>>24)&0xff, crc32) ;

	printf("crc16=%4.4hx, ~crc16=%4.4hx\n", crc16, ~crc16&0xffff) ;
	printf("crc32=%8.8x, ~crc=%8.8x\n", crc32,~crc32) ;
#endif	/* COMMENT */

	exit(0) ;
}


static	int
addcrc(int data, int crc)
{
	int	j ;
	crc ^= (unsigned short) data<<8;
	for(j=0; j<8; ++j) {
	  if( crc & 0x8000 )
	    crc = (crc<<1) ^ 0x1021;
	  else
	    crc <<= 1 ;
	}
	return crc & 0xffff ;
}
