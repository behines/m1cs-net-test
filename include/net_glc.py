# -*- coding: utf-8 -*-

## @file net_glc.py
#    GLC Task and End-point Name Definitions (Ref net_glc.h).
#
#    This module defines all global GLC task names (*_TASK) and
#    component server endpoint names (*_SRV) that are used in conjunction
#    with the Net Services library (e.g., in calls to net_connect()).
#
#  @par Project
#    TMT Primary Mirror Control System (M1CS) \n
#    Jet Propulsion Laboratory, Pasadena, CA
#
#  @author    Thang Trinh
#  @date    28-Jun-2021 -- Initial delivery.
#
#  Copyright (c) 2015-2021, California Institute of Technology
#


## Generic anonymous task

ANON_TASK = 0                # Anonymous client task

## GLC client task names

SEGHCD_TASK = 13             # SEGHCD task

## LSEB server endpoint names

LSEB_CMD_SRV  = "app_srv20"  # LSEB command server
LSEB_CMD_TASK = 120

## I/O mode for Net Services calls

BLOCKING     = 0             # return after I/O completes
NON_BLOCKING = 1             # return immediately, I/O done or not

