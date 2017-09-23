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

# default, libDaoJSON
default: libDaoJSON.a

clean:
	$(RM) default libDaoJSON.a cJSON.o

libDaoJSON.a: cJSON.o
	$(AR) libDaoJSON.a cJSON.o
	$(RANLIB) libDaoJSON.a
	
cJSON.o: cJSON.c cJSON.h
	$(CC) -c cJSON.c
	
.PHONY: default clean

# (end of Makefile)
