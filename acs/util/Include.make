#
# Project:      Thirty Meter Telescope (TMT)
# System:       Primary Mirror Control System (M1CS)
# Task:		Common Utilities (util)
# Module:	Common Utilities Makefile
#
# Author:	T. Trinh, JPL, June 2022
#

# Linux version
#
CC = gcc
CPP = cpp
LD = gcc
AR = ar
CC_INCDIR = /usr/include
RANLIB =

DEFINES = -DLINUX
CFLAGS = -Wall -g -O 
#

LLIBS = 
LDLIBS = -lutil

#

LIB = util

LIB_SRCS = \
	   timer.c

