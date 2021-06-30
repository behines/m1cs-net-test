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
CC = aarch64-none-linux-gnu-gcc
CPP = aarch64-none-linux-gnu-gcc
LD = aarch64-none-linux-gnu-ld
AR = aarch64-none-linux-gnu-ar
# PKG_CONFIG_SYSROOT_DIR is defined in the linux-devkit/environment-setup script.
CC_INCDIR = ${PKG_CONFIG_SYSROOT_DIR}/include/c++/9.2.1/
RANLIB =

DEFINES = -DLINUX
CFLAGS = -Wall -g -O -fPIC
#

LLIBS = 
LDLIBS = -L{PKG_CONFIG_SYSROOT_DIR}/lib -lnetam64 -lc

#

EXES = tstcliam64 

SRCS = tstcli.c 

LIB = netam64

LIB_SRCS = \
	   net_endpt.c \
	   net_io.c \
	   net_tcp.c 

