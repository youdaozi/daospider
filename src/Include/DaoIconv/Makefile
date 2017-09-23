# Utilities.
MKDIR= mkdir -p
RANLIB= ranlib
CC= gcc
#CFLAGS= -O2 -Wall $(MYCFLAGS)
CFLAGS= -O2 -Wall
AR= ar rcu
RM= rm -f
#LIBS= -lm $(MYLIBS)
LIBS= -lm

MYCFLAGS=
MYLIBS=

# default, libDaoIconv
default: libDaoIconv.a

clean:
	$(RM) default libDaoIconv.a DaoIconv.o

libDaoIconv.a: DaoIconv.o
	$(AR) libDaoIconv.a DaoIconv.o
	$(RANLIB) libDaoIconv.a
	
DaoIconv.o: DaoIconv.c DaoIconv.h
	$(CC) -c DaoIconv.c
	
.PHONY: default clean

# (end of Makefile)
