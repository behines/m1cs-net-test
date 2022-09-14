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
LD = g++
AR = ar
CC_INCDIR = /usr/include
RANLIB =

DEFINES = -DLINUX
CFLAGS = -Wall -g -O -Wno-format-overflow
CXXFLAGS = -Wall -g -O -std=c++17
#

LLIBS =
LDLIBS = -lutil -lnet -lnsl

#

EXES = rtc_tstcli lscs_tstsrv

SRCS = rtc_tstcli.cpp HostConnection.cpp lscs_tstsrv.c

