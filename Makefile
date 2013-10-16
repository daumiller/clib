TOP     = $(shell pwd)
DIRSRC  = $(TOP)/source
DIRINC  = $(TOP)/include
DIROBJ  = $(TOP)/object
DIRLIB  = $(TOP)/library
DIRTST  = $(TOP)/test
DIRBIN  = $(TOP)/test/bin
DIRPCRE = /Users/dillon/Development/builds/pcre
DIRINST = /home/dillon/bin/clib.lib
CFLAGS  = -Wall -std=c99 -I$(DIRINC) -DPCREHEADER='"$(DIRPCRE)/include/pcre.h"'
LIBLIB  = $(DIRPCRE)/lib/libpcre.a
TSTLIB  = $(DIRLIB)/clib.a -pthread

all : clib tests

install : clib
	rm -rf $(DIRINST)
	mkdir -p $(DIRINST)/lib
	cp $(DIRLIB)/clib.a $(DIRINST)/lib/
	cp -R $(DIRINC) $(DIRINST)

clib : $(DIRLIB)/clib.a

$(DIRLIB)/clib.a : $(DIROBJ)/string.o     \
                   $(DIROBJ)/regex.o      \
                   $(DIROBJ)/hash.o       \
                   $(DIROBJ)/base64.o     \
                   $(DIROBJ)/list.o       \
                   $(DIROBJ)/sha256.o     \
                   $(DIROBJ)/arguments.o  \
                   $(DIROBJ)/threadPool.o
	mkdir -p $(DIRLIB)
	cd $(DIROBJ);               \
	ar -x $(LIBLIB);            \
	ar rcs $(DIRLIB)/clib.a *.o

$(DIROBJ)/%.o : $(DIRSRC)/%.c
	mkdir -p $(DIROBJ)
	clang $(CFLAGS) $^ -c -o $@

tests : $(patsubst $(DIRTST)/%.c, $(DIRBIN)/%, $(wildcard $(DIRTST)/*.c))

$(DIRBIN)/% : $(DIRTST)/%.c
	mkdir -p $(DIRBIN)
	clang $(CFLAGS) $< $(TSTLIB) -o $@
	strip $@

clean :
	rm -rf $(DIROBJ)

veryclean :
	rm -rf $(DIROBJ)
	rm -rf $(DIRBIN)
	rm -rf $(DIRLIB)

