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
        asock      \
        base64     \
        fileSystem \
        hash       \
        list       \
        regex      \
        sha256     \
        string     \
        threadPool

% : %.c
	$(CC) $(CFLAGS) $< $(CLIBS) -o $@

clean :
	rm -f arguments
	rm -f asock
	rm -f base64
	rm -f fileSystem
	rm -f hash
	rm -f list
	rm -f regex
	rm -f sha256
	rm -f string
	rm -f threadPool

veryclean : clean

