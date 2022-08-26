/**
 *****************************************************************************
 *
 * @file net_ts.h
 *	Timestamp Macro Definitions.
 *
 * @par Project
 *	TMT Primary Mirror Control System (M1CS) \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	24-July-2022 -- Initial delivery.
 *
 * Copyright (c) 2022-2025, California Institute of Technology
 *
 ****************************************************************************/

#ifndef NET_TS_H
#define NET_TS_H

#include <time.h>

//
//  Timestamp macros
//

static struct timespec _currTime;
static struct tm       _tmTime;

#define NET_TIMESTAMP(fmt, ...)  \
		     { clock_gettime (CLOCK_REALTIME, &_currTime);  \
		       gmtime_r(&_currTime.tv_sec, &_tmTime),  \
		       fprintf(stderr, "%04d-%03d-%02d:%02d:%02d.%03ld " fmt,  \
		       _tmTime.tm_year+1900, _tmTime.tm_yday+1,  \
                       _tmTime.tm_hour, _tmTime.tm_min, _tmTime.tm_sec,  \
		       _currTime.tv_nsec/1000000,  \
                       ##__VA_ARGS__); }

#endif /* NET_TS_H */

