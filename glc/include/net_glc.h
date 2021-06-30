/**
 *****************************************************************************
 *
 * @file net_glc.h
 *	GLC Task and End-point Name Definitions.
 *
 *	This header file defines all global GLC task names (*_TASK) and
 *	component server endpoint names (*_SRV) that are used in conjunction 
 *	with the Net Services library (e.g., in calls to net_connect()).
 *
 * @par Project
 *	TMT Primary Mirror Control System (M1CS) \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	28-Jun-2021 -- Initial delivery.
 *
 * Copyright (c) 2015-2021, California Institute of Technology
 *
 ****************************************************************************/

#ifndef NET_GLC_H
#define NET_GLC_H

#include "net_appl.h"

/* Generic anonymous task */

#define ANON_TASK	ANY_TASK	//!< Anonymous client task

/* GLC client task names */

#define SEGHCD_TASK	APP10_TASK	//!< Segment HCD task

/* GLC server task and endpoint names */

/* LSEB server endpoint names */

#define LSEB_CMD_SRV	APP_SRV20	//!< LSEB command server
#define LSEB_CMD_TASK	SRV20_TASK

#endif /* NET_GLC_H */

