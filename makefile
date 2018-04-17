##############################################################################
# File ..................: makefile.linux
# Description ...........: makefile for this project
# Author ................: Peter Thoemmes
#-----------------------------------------------------------------------------
# Copyright (c) 2007-2018, Peter Thoemmes, D-54441 Ockfen/Germany
##############################################################################


##############################################################################
#
# Project:
#
##############################################################################

#
# Name of the resulting executable:
#

APPNAME = heapdump

#
# Name of the object-files to be compiled and linked:
#

OBJS = ConsoleApp.o heapdump.o

#
# Search pathes for source-files:
#

SRC_DIRS = .

#
# File-filter for source-files:
#

SRCS = ./*.cpp

#
# Build this project (*** first *** entry point):
#
#   RELEASE:
#         make -f <makefile>
#         make -f <makefile> all
#

all: $(APPNAME) post_build_proc

#
# Post-build processing:
#

post_build_proc:
	@mkdir -p ./bin
	@mv ./$(APPNAME) ./bin/
	@echo "Build for `uname` `uname -r`" > ./bin/build.info
	@echo done.

##############################################################################
#
# Platform:
#
##############################################################################

#
# Explicit libraries to link:
#
#   Platform                 clock_gettime()  recv(),...  gethostbyname()
#
#   Cygwin 1.5.19-4          -lc
#   RedHat EL 3u4            -lrt
#   SuSE 9.2                 -lrt
#   Solaris 10               -lrt             -lsocket    -lnsl

#EXPLICIT_LIBS = -lc
EXPLICIT_LIBS = -lrt
#EXPLICIT_LIBS = -lrt -lsocket -lnsl

##############################################################################
#
# Development-environment:
#
##############################################################################

#
# Output directory for executables:
#

BIN_DIR = .

#
# Input directory for libraries:
#

LIB_DIRS = -L/usr/lib

#
# C++ compiler:
#

CC = g++

#
# Compiler flags for debuger code:
#
#   -D_DEBUG ................: code with special debugging output
#

CFLAGS_DBG = -D_DEBUG

#
# Compiler flags:
#
#    -D_FILE_OFFSET_BITS=64 ..: use 64 bit file offset for all file operations
#    -D_REENTRANT ............: multithreading code
#    -Wall ...................: warnings on all possible issues (highest level)
#    -g ......................: produce debugging info for gdb (stabs,COFF,...)
#    -m64 ....................: machine-option -> 64 bit
#    -fpic ...................: position independed code (for shared libraries)
#    -static .................: static linking (copies code of lib*.a into app)
#
#    -DRUN_ON_HIGH_PRIORITY ..: code with threads on high priority
#
# REMARK:
# The use of the `-g' switch, which is needed for source-level debugging,
# affects the size of the program executable on disk, and indeed the debugging
# information can be quite large. However, it has no effect on the generated
# code (and thus does not degrade performance)!
#

CFLAGS = -D_FILE_OFFSET_BITS=64 -D_REENTRANT -Wall -g
CFLAGS += -m64
#CFLAGS += -fpic
CFLAGS += -static
#CFLAGS += -DRUN_ON_HIGH_PRIORITY

#
# Includes:
#

INCS = -I.

#
# Libraries:
#

LIBS = $(LIB_DIRS) -pthread $(EXPLICIT_LIBS)

#
# Search path for source-files (needed by the compiler):
#

VPATH = $(SRC_DIRS)

##############################################################################
#
# Entry-points for commandline-parameters to 'make':
#
##############################################################################

#
# Build this project (*** first *** entry point):
#
#   RELEASE:
#         make -f <makefile> release
#
#   DEBUG:
#         make -f <makefile> debug
#

release: $(APPNAME) post_build_proc

debug: CFLAGS += $(CFLAGS_DBG)
debug: $(APPNAME) post_build_proc

#
# Clean this project:
#
#         make -f <makefile> clean
#

clean:
	@rm -rf *.o *.stackdump ./bin/*
	@echo 'done.'

#
# Install the binary:
#
#         make -f <makefile> install
#

install: 
	@if [ "`whoami`" != "root" ]; \
	then echo 'failed: must be root'; \
	else cp -f ./bin/$(APPNAME) /usr/local/bin/; echo 'done.'; \
	fi

#
# Uninstall the binary:
#
#         make -f <makefile> install
#

uninstall:
	@if [ "`whoami`" != "root" ]; \
	then echo 'failed: must be root'; \
	else rm -f /usr/local/bin/$(APPNAME); echo 'done.'; \
	fi

##############################################################################
#
# Linker:
#
##############################################################################

$(APPNAME): $(OBJS)
	$(CC) $(OBJS) -o $(BIN_DIR)/$(APPNAME) $(LIBS)

##############################################################################
#
# Compiler:
#
##############################################################################

.cpp.o:
	$(CC) -c $(INCS) $(CFLAGS) $<
