TOP     = $(shell pwd)
DIRSRC  = $(TOP)/source
DIRINC  = $(TOP)/include
DIROBJ  = $(TOP)/object
DIRLIB  = $(TOP)/library
DIRTST  = $(TOP)/test
DIRPCRE = /Users/dillon/Development/builds/pcre
CFLAGS  = -Wall -std=c99 -I$(DIRINC) -DPCREHEADER='"$(DIRPCRE)/include/pcre.h"'
LIBLIB  = $(DIRPCRE)/lib/libpcre.a
TSTLIB  = $(DIRLIB)/clib.a

all : clib tests

clib : $(DIRLIB)/clib.a

$(DIRLIB)/clib.a : $(DIROBJ)/string.o \
                   $(DIROBJ)/regex.o  \
                   $(DIROBJ)/hash.o   \
                   $(DIROBJ)/base64.o \
                   $(DIROBJ)/list.o   \
                   $(DIROBJ)/sha256.o
	mkdir -p $(DIRLIB)
	cd $(DIROBJ);               \
	ar -x $(LIBLIB);            \
	ar rcs $(DIRLIB)/clib.a *.o

$(DIROBJ)/%.o : $(DIRSRC)/%.c
	mkdir -p $(DIROBJ)
	gcc $(CFLAGS) $^ -c -o $@

tests : clib               \
        $(DIRTST)/regex.c  \
        $(DIRTST)/hash.c   \
        $(DIRTST)/base64.c \
        $(DIRTST)/list.c   \
        $(DIRTST)/string.c \
        $(DIRTST)/sha256.c
	gcc $(CFLAGS) $(DIRTST)/regex.c  $(TSTLIB) -o $(DIRTST)/regex
	gcc $(CFLAGS) $(DIRTST)/hash.c   $(TSTLIB) -o $(DIRTST)/hash
	gcc $(CFLAGS) $(DIRTST)/base64.c $(TSTLIB) -o $(DIRTST)/base64
	gcc $(CFLAGS) $(DIRTST)/list.c   $(TSTLIB) -o $(DIRTST)/list
	gcc $(CFLAGS) $(DIRTST)/string.c $(TSTLIB) -o $(DIRTST)/string
	gcc $(CFLAGS) $(DIRTST)/sha256.c $(TSTLIB) -o $(DIRTST)/sha256
	strip $(DIRTST)/regex
	strip $(DIRTST)/hash
	strip $(DIRTST)/base64
	strip $(DIRTST)/list
	strip $(DIRTST)/string
	strip $(DIRTST)/sha256

clean :
	rm -f $(DIROBJ)/*.o

veryclean :
	rm -rf $(DIROBJ)
	rm -rf $(DIRLIB)
	rm -f  $(DIRTST)/regex
	rm -f  $(DIRTST)/hash
	rm -f  $(DIRTST)/base64
	rm -f  $(DIRTST)/list
	rm -f  $(DIRTST)/string
	rm -f  $(DIRTST)/sha256

