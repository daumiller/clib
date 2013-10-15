TOP     = $(shell pwd)
DIRSRC  = $(TOP)/source
DIRINC  = $(TOP)/include
DIROBJ  = $(TOP)/object
DIRLIB  = $(TOP)/library
DIRTST  = $(TOP)/test
DIRPCRE = /Users/dillon/Development/builds/pcre
DIRINST = /home/dillon/bin/clib.lib
CFLAGS  = -Wall -std=c99 -I$(DIRINC) -DPCREHEADER='"$(DIRPCRE)/include/pcre.h"'
LIBLIB  = $(DIRPCRE)/lib/libpcre.a
TSTLIB  = $(DIRLIB)/clib.a

all : clib tests

install : clib
	rm -rf $(DIRINST)
	mkdir -p $(DIRINST)/lib
	cp $(DIRLIB)/clib.a $(DIRINST)/lib/
	cp -R $(DIRINC) $(DIRINST)

clib : $(DIRLIB)/clib.a

$(DIRLIB)/clib.a : $(DIROBJ)/string.o    \
                   $(DIROBJ)/regex.o     \
                   $(DIROBJ)/hash.o      \
                   $(DIROBJ)/base64.o    \
                   $(DIROBJ)/list.o      \
                   $(DIROBJ)/sha256.o    \
                   $(DIROBJ)/arguments.o
	mkdir -p $(DIRLIB)
	cd $(DIROBJ);               \
	ar -x $(LIBLIB);            \
	ar rcs $(DIRLIB)/clib.a *.o

$(DIROBJ)/%.o : $(DIRSRC)/%.c
	mkdir -p $(DIROBJ)
	clang $(CFLAGS) $^ -c -o $@

tests : clib                  \
        $(DIRTST)/regex.c     \
        $(DIRTST)/hash.c      \
        $(DIRTST)/base64.c    \
        $(DIRTST)/list.c      \
        $(DIRTST)/string.c    \
        $(DIRTST)/sha256.c    \
        $(DIRTST)/arguments.c
	clang $(CFLAGS) $(DIRTST)/regex.c     $(TSTLIB) -o $(DIRTST)/regex
	clang $(CFLAGS) $(DIRTST)/hash.c      $(TSTLIB) -o $(DIRTST)/hash
	clang $(CFLAGS) $(DIRTST)/base64.c    $(TSTLIB) -o $(DIRTST)/base64
	clang $(CFLAGS) $(DIRTST)/list.c      $(TSTLIB) -o $(DIRTST)/list
	clang $(CFLAGS) $(DIRTST)/string.c    $(TSTLIB) -o $(DIRTST)/string
	clang $(CFLAGS) $(DIRTST)/sha256.c    $(TSTLIB) -o $(DIRTST)/sha256
	clang $(CFLAGS) $(DIRTST)/arguments.c $(TSTLIB) -o $(DIRTST)/arguments
	strip $(DIRTST)/regex
	strip $(DIRTST)/hash
	strip $(DIRTST)/base64
	strip $(DIRTST)/list
	strip $(DIRTST)/string
	strip $(DIRTST)/sha256
	strip $(DIRTST)/arguments

clean :
	rm -rf $(DIROBJ)

veryclean :
	rm -rf $(DIROBJ)
	rm -rf $(DIRLIB)
	rm -f  $(DIRTST)/regex
	rm -f  $(DIRTST)/hash
	rm -f  $(DIRTST)/base64
	rm -f  $(DIRTST)/list
	rm -f  $(DIRTST)/string
	rm -f  $(DIRTST)/sha256
	rm -f  $(DIRTST)/arguments

