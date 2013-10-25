DIRINC  = ../../include
DIRLIB  = ../../library
CC      = clang
CFLAGS  = -Wall -std=c99 -I$(DIRINC)
#ifdef __APPLE__
CLIBS   = -pthread $(DIRLIB)/clib.a
#elif __linux__
#CLIBS   = -pthread -lrt $(DIRLIB)/clib.a
#endif

all : tests

debug :
	$(eval CFLAGS +=  -g  )

tests : arguments  \
        base64     \
        sha256     \
        regex      \
        string     \
        threadPool

% : %.c
	$(CC) $(CFLAGS) $< $(CLIBS) -o $@

clean :
	rm -f arguments
	rm -f base64
	rm -f sha256
	rm -f regex
	rm -f string
	rm -f threadPool

veryclean : clean
