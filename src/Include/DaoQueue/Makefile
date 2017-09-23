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

# default, DaoQueue
default: libDaoQueue.a

clean:
	$(RM) default libDaoQueue.a DaoQueue.o

libDaoQueue.a: DaoQueue.o
	$(AR) libDaoQueue.a DaoQueue.o
	$(RANLIB) libDaoQueue.a
	
DaoQueue.o: DaoQueue.c DaoQueue.h
	$(CC) -c DaoQueue.c
	
.PHONY: default clean

# (end of Makefile)
