#

CCARM=arm-buildroot-linux-gnueabihf-gcc
CC=gcc

CFLAGS=-std=std99 -pthread -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

LIBNAME=iniio

SRC=./src
SRCINCL=$(SRC)/

SRCS= 	$(SRC)/iniio.c \

INCLUDEPATH= -I$(SRCINCL)
LIBS=

CC_PARAMS= $(CFLAGS) $(SRCS) -o $(EXECNAME) $(INCLUDEPATH) $(LIBS)

# default build
all:	clean linux

arm:
	@LC_ALL=C $(CCARM) $(CC_PARAMS)

linux:	clean
	@LC_ALL=C $(CC) $(CC_PARAMS)

clean:
	@rm -f ./$(LIBNAME)

.PHONY: all arm linux clean
	
