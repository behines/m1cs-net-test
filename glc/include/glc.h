/**
 *****************************************************************************
 *
 * @file glc.h
 *	GLC Global Declarations.
 *
 *	This header file defines global type definitions and symbolic
 *	constants that are common to all Global Loop Control (GLC) software
 *	components.
 *
 * @par Project
 *	TMT Primary Mirror Control System (M1CS) \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	28-Jun-2021 -- Initial delivery.
 *
 * Copyright (c) 2021, California Institute of Technology
 *
 ****************************************************************************/

#ifndef GLC_H
#define GLC_H

/* standard definitions */

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef ERROR
#define ERROR	(-1)
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef OFF
#define OFF	0
#endif

#ifndef ON
#define ON	1
#endif

#ifndef NULL
#define NULL	0
#endif

#ifndef NULLP
#define NULLP	(char *)0
#endif

#ifndef IN_RANGE
#define IN_RANGE(n,lo,hi) ((lo) <= (n) && (n) <= (hi))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef SWAP
#define SWAP(a,b) ({ long tmp; tmp = (a), (a) = (b), (b) = tmp;})
#endif

extern int sys_nerr;
extern const char * const sys_errlist[];

#ifndef SYS_ERRSTR
#define SYS_ERRSTR(_errno) (((unsigned)_errno > sys_nerr) ? \
    "Illegal errno value" : sys_errlist[_errno])
#endif

/* character definitions */

#ifndef EOS
#define EOS '\000'
#endif

#ifndef BELL
#define BELL '\007'
#endif

#ifndef LF
#define LF '\012'
#endif

#ifndef CR
#define CR '\015'
#endif 

/* integer type definitions */

typedef char	i8, I8;			/*  8-bit integer */
typedef short	i16, I16;		/* 16-bit integer */
typedef int	i32, I32;		/* 32-bit integer */

/* bit and boolean type definitions */
 
typedef unsigned char	  u8, U8;	/*  8 bits, for bitwise operations */
typedef unsigned short	  u16, U16;	/* 16 bits, for bitwise operations */
typedef unsigned int	  u32, U32;	/* 32 bits, for bitwise operations */
typedef unsigned long int u64, U64;	/* 64 bits, for bitwise operations */
 
typedef enum {
    false = 0, true = 1
} boolean;

#endif /* #ifndef GLC_H */

