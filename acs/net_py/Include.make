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
CFLAGS = -O -fPIC
CPPFLAGS += -I/usr/include/python3.8
LDFLAGS += -L/usr/lib/python3.8/lib
#

LLIBS = 
LDLIBS = -lpython3.8 -Wl,--whole-archive -lnet -Wl,--no-whole-archive

#

EXES =

SRCS =

SHLIB = netpy

LIB_SRCS = netpy.c

