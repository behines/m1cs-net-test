#
# Project:	Thirty Meter Telescope (TMT)
# System:	Primary Mirror Control System (M1CS)
# Task:		Network Communication Services (net)
# Module:	Network Communication Services Makefile
#
# Author:	T. Trinh, JPL, June 2021
#

#TARGET options am335x am64x msp432 c2000 
TARGET= 
-include $(ROOTDIR)/Include.tools

# DEFINES: list of preprocessor flags to set when compiling source
DEFINES = -DLINUX

# CFLAGS: standard C compilier flags
CFLAGS = --sysroot=$(SYSROOT) -Wall -g -O -fPIC -Wno-format-overflow 

# CXXFLAGS: standard C++ compilier flags to set
CXXFLAGS = --sysroot=$(SYSROOT) -Wall -g -O  -Wno-format-overflow -std=c++17

# LLIBS: local project libraries to linked to EXE.
LLIBS = 

# LDLIBS: system libraries/library paths to linked to EXE
LDLIBS = -lpthread -lrt  --sysroot=$(SYSROOT)

# EXES: name of executable(s) to be created.
EXES = rtc_tst_udp lscs_tst_udp 

# SRCS: list of source files to be compiled/linked with EXE.o 
SRCS = PThread.cpp UdpConnection.cpp Server.cpp Client.cpp IpInterfaceInfo.cpp

#rtc_udp.cpp UdpConnection.cpp Server.cpp Client.cpp PThread.cpp lscs_udp.cpp
# LIB: Name of library to be created.
LIB = 

# LIB_SRCS: list of source files to be compiled and linked into LIB
LIB_SRCS =  

