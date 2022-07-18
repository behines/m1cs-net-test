/**
 *****************************************************************************
 *
 * @file timer.c 
 *   Timer routines - See sample code test() below. 
 *
 * @par Project
 *	LCRD Optical Ground Station (OGS-1)  \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author Minh Lang
 * @date   02-June-2016 - build 2
 *   
 *  svn tags:
 *  $Id: timer.c 1956 2018-12-17 21:55:25Z mcscm $
 *
 *   History:
 *     03/21/2018, Minh Lang: make timer fd non-blocking so that read() won't
 *                            block if timer did not expire.
 *     04/07/2016, Minh Lang: setTimer() now takes struct timeval as params
 *     09/11/2015, Minh Lang: created
 *
 * Copyright (c) 2015, California Institute of Technology
 *
 *****************************************************************************
 */

#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>      /* Definition of uint64_t */
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>

#include "timer.h"

typedef int     i32, I32;	/* 32-bit integer */


/**
 * @fn unsigned long long GET_TIME()
 * @par return current system time in microseconds
 * @param[in]  NONE
 * @return: system time in microseconds
 *
 */
unsigned long long GET_TIME()
{
   struct timeval tv;
   struct timezone tz;

   gettimeofday(&tv, &tz);
   return (unsigned long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * @fn I32 timerCreate (I32 *fd)
 * @par   create a new timer fd
 * @param[in/out]  *fd : pointer to int file descriptor
 * @return: 0 or -1 (ERROR)
 *
 */
I32 timerCreate (I32 *fd)
{
   I32 retVal = 0;

   if ((*fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK)) == -1)
   {
      fprintf (stderr, "Error creating timerfd. aborted\n");
      retVal = -1; 
   }
   return retVal;
}

/**
 *  @fn I32 setTimer (I32 _timerFd, struct timeval *oneShotVal, struct timeval *periodicVal)
 *  @par  set timer expiration to either one-shot or periodic. Set both to 0 to disable timer activation.
 * @param[in]  oneShotSpec : struct timespec where tv_sec and tv_nsec set the time.
 * @param[in]  periodicspec: struct timespec for the timer to go off repeatedly (optional)
 *                          after the timer was triggered in 'oneShotSpec'.
 * @return  Error code
 * @retval  0 (SUCCESS)
 * @retval  -1 (ERROR)
 *
 */
I32 setTimer (I32 _timerFd, struct timeval *oneShotVal, struct timeval *periodicVal)
{
   struct itimerspec timeVal;
   I32 oneShotSec, oneShotNsec;
   I32 periodicSec, periodicNsec;
   I32 retVal = 0;

   if (oneShotVal == NULL)
      oneShotSec = oneShotNsec = 0;
   else
   {
      oneShotSec = oneShotVal->tv_sec;
      oneShotNsec = oneShotVal->tv_usec * 1000;
   }

   if (periodicVal == NULL)
      periodicSec = periodicNsec = 0;
   else
   {
      periodicSec  = periodicVal->tv_sec;
      periodicNsec = periodicVal->tv_usec * 1000;
   }

   timeVal.it_value.tv_sec     = oneShotSec;
   timeVal.it_value.tv_nsec    = oneShotNsec;
   timeVal.it_interval.tv_sec  = periodicSec;
   timeVal.it_interval.tv_nsec = periodicNsec;

   if (timerfd_settime (_timerFd, 0, &timeVal, NULL) == -1)
   {
      fprintf (stderr, "Error setting timerfd. aborted\n");
      retVal = -1;
   }
   return retVal;
}

/**
 *  @fn I32 resetTimer (I32 _timerFd)
 *  @par  after timer expired, it needs to be cleared. Otherwise, it would keep tripping
 *
 * @param[in]  _timerFd : previously created timer file descriptor
 *
 * @return  Error code
 * @retval  0 (SUCCESS)
 * @retval  -1 (ERROR)
 *
 */
I32 resetTimer (I32 _timerFd)
{
   I32 retVal = 0;
   unsigned long long missed;

   if (read (_timerFd, &missed, sizeof(missed)) == -1) // clear the timer
   {
      retVal = -1;
   }
   return retVal;
}

