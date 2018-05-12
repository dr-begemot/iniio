CC=arm-buildroot-linux-gnueabihf-gcc
CCLINUX=gcc

CFLAGS=-std=gnu11 -pthread -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function
#-Wextra 
TAGERTIP=192.168.3.151

EXECNAME=iniio

# PATHS
#SDKBASE=/home/user/dev/ti_sdk
#SDKINCL=$(SDKBASE)/usr/include
SRC=./src
SRCINCL=$(SRC)/

SRCS= 	$(SRC)/main.c \
	$(SRC)/iniio.c \

INCLUDEPATH= -I$(SRCINCL)

CC_PARAMS= $(CFLAGS) $(SRCS) -o $(EXECNAME) $(INCLUDEPATH)

# default build
all:	clean linux

arm:
	@LC_ALL=C $(CC) $(CC_PARAMS)

linux:	clean
	@LC_ALL=C $(CCLINUX) -DRUN_ON_PC $(CC_PARAMS)

clean:
	@rm -f ./$(EXECNAME)

.PHONY: all arm linux clean
