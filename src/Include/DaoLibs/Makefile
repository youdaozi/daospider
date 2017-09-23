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

# default, DaoLibs
default: libDaoLibs.a

clean:
	$(RM) default libDaoLibs.a DaoLibs.o

libDaoLibs.a: DaoLibs.o
	$(AR) libDaoLibs.a DaoLibs.o
	$(RANLIB) libDaoLibs.a
	
DaoLibs.o: DaoLibs.c DaoLibs.h
	$(CC) -c DaoLibs.c
	
.PHONY: default clean

# (end of Makefile)
