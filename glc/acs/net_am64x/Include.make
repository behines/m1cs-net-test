#
# Project:	Thirty Meter Telescope (TMT)
# System:	Primary Mirror Control System (M1CS)
# Task:		Network Communication Services (net)
# Module:	Network Communication Services Makefile
#
# Author:	T. Trinh, JPL, June 2021
#

# PKG_SYSROOT_DIR is defined in the linux-devkit/environment-setup script.
SYSROOT_DIR=
TOOLCHAIN_PREFIX=
TARGET_SYS=
SYSROOT_DIR=/opt/ti/processor_sdk_linux_am64x_07_03_01_006/linux-devkit/sysroots/aarch64-linux
TOOLCHAIN_PREFIX=aarch64-none-linux-gnu-
TARGET_SYS=_am64x

# Linux version
#
CPP = $(TOOLCHAIN_PREFIX)gcc -E
CC = $(TOOLCHAIN_PREFIX)gcc 
CXX = $(TOOLCHAIN_PREFIX)g++ 
LD = $(TOOLCHAIN_PREFIX)gcc
AR = $(TOOLCHAIN_PREFIX)ar
RANLIB = $(TOOLCHAIN_PREFIX)ranlib
STRIP = $(TOOLCHAIN_PREFIX)strip
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
AS = $(TOOLCHAIN_PREFIX)as
AR = $(TOOLCHAIN_PREFIX)ar
NM = $(TOOLCHAIN_PREFIX)nm
GDB = $(TOOLCHAIN_PREFIX)gdb


DEFINES = -DLINUX
#CPATH=$(SYSROOT_DIR)/usr/include:
CC_INCDIR=$(SYSROOT_DIR)/usr/include
CFLAGS = --sysroot=$(SYSROOT_DIR) -Wall -g -O -fPIC 
#

LLIBS = 
LDLIBS = -L$(SYSROOT_DIR)/lib -lnet$(TARGET_SYS)

#

EXES = tstcli tstsrv

SRCS = 

LIB = net$(TARGET_SYS)

LIB_SRCS = \
	   net_endpt.c \
	   net_io.c \
	   net_tcp.c 

