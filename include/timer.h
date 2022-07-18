/**
 *****************************************************************************
 * @file timer.h 
 * Header file for watchdog timers.
 *
 * @par Project
 *	LCRD Optical Ground Station (OGS-1)  \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author Minh Lang
 * @date   02-June-2016 - build 2
 *
 *  History:
 *   09/16/2015 minhlang : created
 *
 *  svn tags:
 *  $Revision: 280 $
 *  $Id: timer.h 286 2016-06-10 02:48:36Z mcscm $
 *  
 * Copyright (c) 2015, California Institute of Technology
 *
 *****************************************************************************
 * */

unsigned long long GET_TIME();
int timerCreate (int *fd);
int setTimerOld (int _timerFd, int oneShotSec, int periodicSec);
int setTimer (int _timerFd, struct timeval *oneShotSec, struct timeval *periodicSec);
int resetTimer (int _timerFd);
