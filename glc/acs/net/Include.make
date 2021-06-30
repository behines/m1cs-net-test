#
# Project:	Thirty Meter Telescope (TMT)
# System:	Primary Mirror Control System (M1CS)
# Task:		Network Communication Services (net)
# Module:	Network Communication Services Makefile
#
# Author:	T. Trinh, JPL, June 2021
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
CFLAGS = -Wall -g -O -fPIC
#

LLIBS = 
LDLIBS = -lnet 

#

EXES = tstcli 

SRCS = tstcli.c 

LIB = net

LIB_SRCS = \
	   net_endpt.c \
	   net_io.c \
	   net_tcp.c 

