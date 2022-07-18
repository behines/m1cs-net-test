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
 *	TMT Primary Mirror Control System (M1CS)
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	28-Jun-2021 -- Initial delivery.
 *
 * Copyright (c) 2015-2022, California Institute of Technology
 *
 ****************************************************************************/

#ifndef NET_GLC_H
#define NET_GLC_H

#include "net_appl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generic anonymous task */

#define ANON_TASK	(ANY_TASK)	//!< Anonymous client task

/* GLC client task names */

#define SEGHCD_TASK	(APP10_TASK)	//!< Segment HCD task
#define DIAGMON_TASK	(APP11_TASK)	//!< LSCS Diagnostic Monitor task

/* GLC server task and endpoint names */

#define M1RTC_TASK      (APP1_TASK)	//!< M1 RTC Command Interface server task
#define M1RTC_SRV       (APP_SRV3)

/* LSCS server endpoint names */

#define LSCS_50HZ_DATA_SRV    (APP_SRV19)	//!< LSCS 50Hz Data Server
#define LSCS_50HZ_DATA_TASK   (SRV19_TASK)

#define LSCS_CMD_SRV          (APP_SRV20)	//!< LSCS Command Server
#define LSCS_CMD_TASK         (SRV20_TASK)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_GLC_H */

