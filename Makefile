TOP     = $(shell pwd)
DIRSRC  = $(TOP)/source
DIRINC  = $(TOP)/include
DIROBJ  = $(TOP)/object
DIRLIB  = $(TOP)/library
DIRTST  = $(TOP)/test
DIRBIN  = $(TOP)/test/bin
CC      = clang
CFLAGS  = -Wall -std=c99 -I$(DIRINC) -DPCREHEADER='"$(DIRPCRE)/include/pcre.h"'
STRIP   = strip
#ifdef __APPLE__
DIRPCRE = /Users/dillon/Development/builds/pcre
DIRINST = /Users/dillon/Development/builds/clib/install
TSTLIB  = $(DIRLIB)/clib.a -pthread
#elif __linux__
#DIRPCRE = /home/dillon/bin/pcre.lib
#DIRINST = /home/dillon/bin/clib.lib
#TSTLIB  = $(DIRLIB)/clib.a -pthread -lrt
#endif
LIBLIB  = $(DIRPCRE)/lib/libpcre.a

all : clib tests

debug :
	$(eval CFLAGS +=  -g  )
	$(eval STRIP   = touch)

install : clib
	rm -rf $(DIRINST)
	mkdir -p $(DIRINST)/lib
	cp $(DIRLIB)/clib.a $(DIRINST)/lib/
	cp -R $(DIRINC) $(DIRINST)

clib : $(DIRLIB)/clib.a

$(DIRLIB)/clib.a : $(DIROBJ)/string.o      \
                   $(DIROBJ)/regex.o       \
                   $(DIROBJ)/hash.o        \
                   $(DIROBJ)/base64.o      \
                   $(DIROBJ)/list.o        \
                   $(DIROBJ)/sha256.o      \
                   $(DIROBJ)/arguments.o   \
                   $(DIROBJ)/threadPool.o  \
                   $(DIROBJ)/asockWorker.o \
                   $(DIROBJ)/asock_sync.o  \
                   $(DIROBJ)/asock_async.o \
                   $(DIROBJ)/fileSystem.o
	mkdir -p $(DIRLIB)
	cd $(DIROBJ);               \
	ar -x $(LIBLIB);            \
	ar rcs $(DIRLIB)/clib.a *.o

$(DIROBJ)/%.o : $(DIRSRC)/%.c
	mkdir -p $(DIROBJ)
	$(CC) $(CFLAGS) $^ -c -o $@

tests : $(patsubst $(DIRTST)/%.c, $(DIRBIN)/%, $(wildcard $(DIRTST)/*.c))

$(DIRBIN)/% : $(DIRTST)/%.c
	mkdir -p $(DIRBIN)
	$(CC) $(CFLAGS) $< $(TSTLIB) -o $@
	$(STRIP) $@

clean :
	rm -rf $(DIROBJ)

veryclean :
	rm -rf $(DIROBJ)
	rm -rf $(DIRBIN)
	rm -rf $(DIRLIB)

