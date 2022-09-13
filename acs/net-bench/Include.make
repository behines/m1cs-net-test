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
CFLAGS = -Wall -g -O -Wno-format-overflow
CXXFLAGS = -Wall -Werror -g -O -std=c++17
#

LLIBS =
LDLIBS = -lutil -lnet -lnsl

#

EXES = rtc_tstcli

SRCS = rtc_tstcli.cpp

