#
#
# Project:		Thirty Meter Telescope (TMT)
# System:		Primary Mirror Control System (M1CS)
# Module:		Global Makefile - Adapted from SCDU Project
# Authors:		T. Trinh, JPL, June 2021
#			G. Brack, JPL, Apr 1995
#			T. Truong, JPL, Nov 1993; T. Trinh, JPL, Oct 1992
#
# Usage:
#             
#   		From any source directory:
#		make -f $HOME/m1cs/glc/Makefile
#
#   		Create dependencies listing:
#		make -f $HOME/m1cs/glc/Makefile depend
#
#   		Remove object files:
#		make -f $HOME/m1cs/glc/Makefile clean
#
# Notes: 
#	This makefile requires GNU make.
#
# Discription:
#	The global makefile is designed to work from any of the subdirectories
#       within the M1CS system.  By default it defines flags that would be
#       correct for Linux.  The makefile looks for 2 files in the current
#	directory when compiling, Include.make, and .depend.make.  When the
#	makefile is processed by make it first defines all of the global
#	macros.  Then it looks at Include.make to override any of the
# 	global macros.  After processing Include.make it returns to this
#	file to define the global compilation macros used to create
#	object modules, and executables.  Finally if .depend.make exists
#	make processes it to determine which files should be recompiled.
#	So when finished the makefile could be thought of as a single file
#	that looks like:
#
#		---------------------------------------
#		|                                     |
#		|  Global Execution Commands          |
#		|                                     |
#		---------------------------------------
#		|                                     |
#		|  Include.make in current directory  |
#		|                                     |
#		---------------------------------------
#		|                                     |
#		|  Global Compilation Commands        |
#		|                                     |
#		---------------------------------------
#		|                                     |
#		|  .depend.make                       |
#		|                                     |
#		---------------------------------------
#
#


# tell make directories to search for header file dependencies
vpath %.h ../include
vpath %.h ../../include
# tell make directories to search for library dependencies 
vpath %.a ../lib
vpath %.a ../../lib

ifeq ($(ROOTDIR),)
    ROOTDIR=${HOME}/m1cs-net-test
endif

# Global directories
BINDIR= ${ROOTDIR}/bin
LIBDIR= ${ROOTDIR}/lib
BSPDIR= 

# Execution Commands
RM = /bin/rm -f
OS:= $(shell uname)
ifeq ($(OS),SunOS)
    MAKE= /usr/local/bin/make -f ${ROOTDIR}/Makefile
    LINT= /opt/SUNWspro/bin/lint
    YACC= /usr/ccs/bin/yacc
    LEX= /usr/ccs/bin/lex
    INSTALL= /usr/local/bin/install
    RANLIB=
    CPP=/usr/ccs/lib/cpp
    LD= /opt/SUNWspro/bin/cc
    AR= /usr/ccs/bin/ar
    AS= /usr/ccs/bin/as
    CC= /opt/SUNWspro/bin/cc
    CXX= /opt/SUNWspro/bin/CC
else
    MAKE= /usr/bin/make -f ${ROOTDIR}/Makefile
    LINT=				# Currently don't have lint on Linux
    YACC= /usr/bin/yacc
    LEX= /usr/bin/lex
    INSTALL= /usr/bin/install
    RANLIB=
    CPP=/usr/bin/cpp
    LD= /usr/bin/cc
    AR= /usr/bin/ar
    AS= /usr/bin/as
    CC= /usr/bin/cc
    CXX= /usr/bin/CC
endif

INCLUDE=.
CC_INCDIR=/usr/include			# Compiler standard header files
DEFINES= -DSVR4
YFLAGS= -d
CFLAGS= -g 
LDFLAGS= -L../lib -L$(LIBDIR)

CPPFLAGS= $(DEFINES) -I$(INCLUDE) -I../include -I../../include -I$(CC_INCDIR)
CXXFLAGS= $(DEFINES) -I$(INCLUDE) -I../include -I../../include -I$(CC_INCDIR)
LINTFLAGS= -u -Ncheck=macro -Nlevel=4

-include Include.make

MDEPEND=.depend.make
FILE= $(MDEPEND)

.l.c:
	@( $(RM) $@; \
	$(LEX) -t $< > $@)
.y.c:
	@($(YACC.y) $<; \
	mv y.tab.c $@ )


# Global Compilation Macros
DEPEND_SRCS= [ "$(SUBDIRS)" ] && : || echo "$(SRCS) $(LIB_SRCS)"

.INIT:	$(FILE)

all: $(LIB:%=$(LIBDIR)/lib%.a) $(SHLIB:%=$(LIBDIR)/%.so) $(EXES:%=$(BINDIR)/%)
	@(D="$(SUBDIRS)";                                \
	for i in $$D; do (cd $$i;                       \
	    $(MAKE) $@ );                               \
	done)

# FIX: Minor bug that one executable depends on other executable resulting
#      in case that when multiple executables are in the same directory they
#      always reinstall.  
$(EXES:%=$(BINDIR)/%): $(EXES)
	@(E="$(EXES)";					\
	for i in $$E; do ( 				\
	    $(INSTALL) -m 755 $$i $(BINDIR);		\
	    (if [ -d ../bin ];then $(INSTALL) -m 755 $$i ../bin;fi) \
	);						\
	done)					


$(EXES): $(ROOTDIR)/Makefile Include.make
$(EXES): $(LIB:%=lib%.a) 
$(EXES): $(LLIBS:-l%=lib%.a) 
$(EXES): $(filter %.o,$(SRCS:%.c=%.o)) 
$(EXES): $(filter %.o,$(SRCS:%.C=%.o)) $(filter %.o,$(SRCS:%.s=%.o)) 
	$(LD) -o $@ $(filter %.o,$(SRCS:.c=.o)) \
	$(filter %.o,$(SRCS:.C=.o)) $(filter %.o,$(SRCS:.s=.o)) \
	$(LDFLAGS) $(LLIBS) $(LDLIBS); 
	@(if [ "$(XRT_AUTH)" ]; then $(XRT_AUTH) $@; fi;)

$(LIB:%=lib%.a): $(filter %.o,$(LIB_SRCS:%.c=%.o) ) 
$(LIB:%=lib%.a): $(filter %.o,$(LIB_SRCS:%.C=%.o) ) 
$(LIB:%=lib%.a): $(filter %.o,$(LIB_SRCS:%.s=%.o) ) 
	$(AR) rcv $(LIB:%=lib%.a) $?
	@(if [ "$(RANLIB)" ]; then $(RANLIB) $@; fi;)

$(LIB:%=$(LIBDIR)/lib%.a) : $(LIB:%=lib%.a)
	@(if [ -d ../lib ]; then $(INSTALL) -m 644 $? ../lib; fi )
	@($(INSTALL) -m 644 $? $(LIBDIR))

$(SHLIB:%=%.so): $(filter %.o,$(LIB_SRCS:%.c=%.o) )
$(SHLIB:%=%.so): $(filter %.o,$(LIB_SRCS:%.C=%.o) )
$(SHLIB:%=%.so): $(filter %.o,$(LIB_SRCS:%.s=%.o) )
	$(LD) -shared $(LDFLAGS) $(LLIBS) $(LDLIBS) -o $(SHLIB:%=%.so) $?

$(SHLIB:%=$(LIBDIR)/%.so) : $(SHLIB:%=%.so)
	@(if [ -d ../lib ]; then $(INSTALL) -m 755 $? ../lib; fi )
	@($(INSTALL) -m 755 $? $(LIBDIR))

$(MDEPEND):
	@touch $(MDEPEND)
 
# FIX: depend does not handle case when EXES main routine is in a C++ file
depend: FORCE
depend: $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS))
depend: $(filter %.C,$(SRCS)) $(filter %.C,$(LIB_SRCS)) 
	@(D="$(SUBDIRS)";                                                 \
	for i in $$D; do                                                  \
	    (cd $$i; $(MAKE) $@ )                                         \
	done)
	@(F="$?";                                                         \
	if [ "$$F" -a $(PWD) != "'pwd'" ]; then echo "make depend:"; fi;  \
	for i in $$F; do                                                  \
	    if [ "$$i" != "FORCE" ]; then $(CPP) $(CPPFLAGS) -MM $$i; fi;  \
	done  > $(MDEPEND))
#	@chmod -f 664 $(MDEPEND)

FORCE:

docs:
	cd ${ROOTDIR}/doc; \
	echo "Delete previous files - if they exist"; \
	/bin/rm -rf html/*; \
	echo "Start processing...  *** THIS MAY TAKE AWHILE! ***"; \
	/usr/bin/doxygen  Doxyfile . ;

# FIX multiple definition of main when have multiple executibles
lint: $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) $(EXES:%=%.c)
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(EXES:%=%.c) \
	$(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) >linterrs 2>&1

# FIX multiple definition of main when have multiple executibles
tags:  $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) $(EXES:%=%.c)
	ctags -x $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) \
	$(EXES:%=%.c) > tags

# FIX multiple definition of main when have multiple executibles
etags:  $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) $(EXES:%=%.c)
	etags -I -o TAGS $(filter %.c,$(SRCS)) $(filter %.c,$(LIB_SRCS)) \
	$(EXES:%=%.c)

clean: FORCE
	@(D="$(SUBDIRS)";				\
	for i in $$D; do 				\
	    (cd $$i; $(MAKE) $@ )			\
	done)
	$(RM) *.o core 

xclean: FORCE
	@(D="$(SUBDIRS)";				\
	for i in $$D; do 				\
	    (cd $$i; $(MAKE) $@ )			\
	done)
	$(RM) *.o core *.ln linterrs $(EXES) $(LIB:%=lib%.a) $(EXES:%=../bin/%) $(LIB:%=../lib/lib%.a) $(EXES:%=$(BINDIR)/%) $(LIB:%=$(LIBDIR)/lib%.a) 

debug:  FORCE
	@$(MAKE) ROOTDIR=$(ROOTDIR) CFLAGS="$(CFLAGS) -g" LDFLAGS="$(LDFLAGS) -g"

gprof: FORCE
	@$(MAKE) ROOTDIR=$(ROOTDIR) CFLAGS="$(CFLAGS) -pg" LDFLAGS="$(LDFLAGS) -g"

include $(MDEPEND)
