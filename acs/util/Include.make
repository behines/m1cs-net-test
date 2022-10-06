#
# Project:	Thirty Meter Telescope (TMT)
# System:	Primary Mirror Control System (M1CS)
# Task:		Common Services 
# Module:	
#
# Author:
#

#
# Supported TARGET values: am335x, am64x, msp432, c2000, and empty for native host 
#
TARGET=

# System wide compilers
-include $(ROOTDIR)/Include.tools

# DEFINES: list of preprocessor flags to set when compiling source
DEFINES = -DLINUX

# CFLAGS: standard C compilier flags
CFLAGS = -Wall -Werror -g -O -fPIC 

# CXXFLAGS: standard C++ compilier flags to set
CXXFLAGS = -Wall -Werror -g -O -fPIC 

# LLIBS: local project libraries to linked to EXE.
LLIBS =  

# LDLIBS: system libraries/library paths to linked to EXE
LDLIBS = 

# EXES: name of executable(s) to be created.
EXES = StructSize AnalyzeNetTest

# SRCS: list of source files to be compiled/linked with EXE.o 
SRCS =  SegmentData.cpp

# LIB: Name of library to be created.
LIB = util

# LIB_SRCS: list of source files to be compiled and linked into LIB
LIB_SRCS = timer.c

