#
#  MPI Bandwidth and Latency Benchmark Makefile
#
#  Modify the CC, LIBS, CFLAGS, LDFLAGS, LIBDIR, and INCDIR values,
#    if necessary, to compile the test applications for the target system
#

#  Default values
MPIHOME=
CC=$(MPIHOME)/bin/mpicc
DISTRIB=
STACK_PREFIX=
LIBS= -lm -L$(MPIHOME)/lib/shared -L$(MPIHOME)/lib -L$(DISTRIB)/$(STACK_PREFIX)/lib64 -L$(DISTRIB)/$(STACK_PREFIX)/lib
CFLAGS= -O2 -g -I$(MPIHOME)/include
LDFLAGS= 
INCDIR=

#  Setting for using gcc for lint
LINT=
ifdef LINT
LINTFLAGS=-ansi -pedantic -W -Wall -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -O2
CFLAGS:=$(LINTFLAGS) $(CFLAGS)
CC=mpicc
endif

MSG  = com glob
EXES = $(MSG) globalop

SRCS = com.c glob.c glob.h globalop util.c util.h
PACKAGE_FILES = Makefile README $(SRCS)

all def: $(EXES)

msg: $(MSG)

util.o com.o :: util.h

.c.o: util.h
	$(CC) $(CFLAGS) $(INCDIR) -c $<

com: com.o util.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ com.o util.o $(LIBS)

glob: glob.o util.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ glob.o util.o $(LIBS)

globalop: globalop.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ globalop.o $(LIBS)

clean:
	rm -f $(EXES) *.o


distclean: clean
	rm -f *~ *.out

dist: distclean
	tar cvf presta.tar $(PACKAGE_FILES)
