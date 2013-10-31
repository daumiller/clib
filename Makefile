TOP     = $(shell pwd)
DIRSRC  = $(TOP)/source
DIRINC  = $(TOP)/include
DIROBJ  = $(TOP)/object
DIRLIB  = $(TOP)/library
DIRTST  = $(TOP)/test
DIRBIN  = $(TOP)/test/bin
CC      = clang
CFLAGS  = -Wall -std=c99 -I$(DIRINC) -DPCREHEADER='"$(DIRPCRE)/include/pcre.h"' -D_CLIB_SSH_
STRIP   = strip

UNAME_S = $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  # __APPLE__
  DIRINST = /Users/dillon/Development/builds/clib/install
  DIRPCRE = /Users/dillon/Development/builds/pcre
  DIRSSH2 = /Users/dillon/Development/builds/libssh2
  DIROSSL = /Users/dillon/Development/builds/openssl
  DIROSSLIB = $(DIROSSL)/lib
  TSTLIB  = $(DIRLIB)/clib.a -pthread -lz
endif
ifeq ($(UNAME_S),Linux)
  # __linux__
  DIRINST = /home/dillon/bin/clib.lib
  DIRPCRE = /home/dillon/bin/pcre.lib
  DIRSSH2 = /home/dillon/bin/libssh2.lib
  DIROSSL = /home/dillon/bin/openssl.lib
  DIROSSLIB = $(DIROSSL)/lib64
  TSTLIB  = $(DIRLIB)/clib.a -pthread -lrt -ldl -lz
endif
$(eval CFLAGS += -I$(DIRSSH2)/include)

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
	mkdir -p $(DIROBJ)/ingest
	cd $(DIROBJ)/ingest;            \
	ar -x $(DIRPCRE)/lib/libpcre.a; \
	ar -x $(DIRSSH2)/lib/libssh2.a; \
	ar -x $(DIROSSLIB)/libssl.a;    \
	ar -x $(DIROSSLIB)/libcrypto.a; \
	cd ..;                          \
	ar rcs $(DIRLIB)/clib.a *.o $(DIROBJ)/ingest/*.o

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

remake : veryclean all
