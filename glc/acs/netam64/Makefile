#
# Project:      Thirty Meter Telescope (TMT)
# System:       Primary Mirror Control System (M1CS)
# Task:         Network Communication Services (net)
# Module:       Network Communication Services Makefile
#
# Author:       Thang Trinh, JPL, Oct 1993
#

# PKG_SYSROOT_DIR is defined in the linux-devkit/environment-setup script.                                       
SYSROOT_DIR=/
TOOLCHAIN_PREFIX=
TARGET_SYS=
SYSROOT_DIR=/opt/ti/processor_sdk_linux_am64x_07_03_01_006/linux-devkit/sysroots/aarch64-linux
TOOLCHAIN_PREFIX=aarch64-none-linux-gnu-
TARGET_SYS=_am64x

# Linux version                                                                                                  

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
#CONFIGURE_FLAGS=--target=aarch64-linux --host=aarch64-linux --build=x86_64-linux --with-libtool-sysroot=$(SYSROOT_DIR)



INSTALLDIR = ../..

BINDIR = $(INSTALLDIR)/bin
LIBDIR = $(INSTALLDIR)/lib
MANDIR = $(INSTALLDIR)/man

.INIT: depend.make

# .PRECIOUS:

CFLAGS =	--sysroot=$(SYSROOT_DIR) -g

DEFINES = 	-DLINUX
CPATH=$(SYSROOT_DIR)/usr/include:

CPPFLAGS =	$(DEFINES) \
		-I../../include

LDFLAGS =	--sysroot=$(SYSROOT_DIR) -L$(LIBDIR)

LDLIBS =	

#

LINT=

LLIBS = libnet$(TARGET_SYS).a

LINTFLAGS = -u $(DEFINES) $(LLIBS)

#

LIB =	libnet$(TARGET_SYS).a

INCLUDES =

LIB_SRCS = \
	net_endpt.c \
	net_io.c \
	net_tcp.c 
#	net_bcst.c

LIB_OBJS = \
	$(LIB_SRCS:.c=.o)

EXES =	tstcli$(TARGET_SYS) 
#EXES =  tstsrv
#

SRCS = tstcli.c 
#SRCS = tstsrv.c	

OBJS =	$(LIB_OBJS)

MANS = 

#

all:	$(INCLUDES) $(SRCS) $(LIBDIR)/$(LIB) $(EXES) $(MANS)

install: $(LIBDIR)/$(LIB) $(EXES)
	install -m 755 $(EXES)$(TARGET_SYS) $(BINDIR) 

$(LIBDIR)/$(LIB): $(LIB)
	install -m 644 $(LIB) $(LIBDIR) 

$(LIB): $(LIB_OBJS)
	$(AR) rcv $@ $?

$(EXES): Makefile Include.make
#$(EXES): $(LIB:%=lib%.a)
$(EXES): $(LLIBS:-l%=lib%.a)
$(EXES): $(filter %.c,$(EXES:%$(TARGET_SYS)=%.c))
$(EXES): $(filter %.o,$(SRCS:%.c=%.o))
$(EXES): $(filter %.o,$(SRCS:%.C=%.o)) 
	$(CC) -o $@  $(filter %.o,$(SRCS:.c=.o)) $(filter %.o,$(SRCS:.C=.o)) \
	$(LDFLAGS) $(LLIBS) $(LDLIBS); 


man: $(MANS)

doc:

clean::
	@rm -f *~
	@rm -f core
	rm -f *.o
	rm -f $(LIB)
	rm -f $(EXES)

clear:	clean
	-sccs clean

# C source targets

debug:
	$(MAKE) 'CFLAGS=$(CFLAGS) -g' 'LDFLAGS=$(LDFLAGS) -g'

lint:	$(INCLUDES) $(SRCS)
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(SRCS) >linterrs 2>&1

gprof:
	$(MAKE) 'CFLAGS=$(CFLAGS) -pg' 'LDFLAGS=$(LDFLAGS) -pg'

depend: $(INCLUDES) $(SRCS)
	(for i in $(SRCS) ; do $(CPP) $(CPPFLAGS) -M $$i ; done) | \
	grep -v "/usr/include" | sort | uniq >depend.make

depend.make:
	@touch depend.make

include depend.make

