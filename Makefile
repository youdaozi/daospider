#dir
INSTALL_TOP=/usr/local/soft/daoscollection/
MAKE_TOP= ./release
MAKE_BIN= $(MAKE_TOP)/bin
MAKE_CONF= $(MAKE_TOP)/conf
MAKE_DB= $(MAKE_TOP)/db
MAKE_INC= $(MAKE_TOP)/include
MAKE_LIB= $(MAKE_TOP)/lib
MAKE_LUALIB= $(MAKE_TOP)/lualib
MAKE_DOC= $(MAKE_TOP)/doc

# Utilities.
MYLIBS=-ldl -lmysqlclient_r -liconv -lm -lcurl -lpcre -pthread -ldb -lsqlite3 -lDaoLibs -lDaoQueue -lDaoIconv -lDaoJSON -lDaoLua
MYCFLAGS=-DDSCROOTPATH=\"$(INSTALL_TOP)\"

MKDIR= mkdir -p
RANLIB= ranlib
AR= ar rcu
RM= rm -f
CC= gcc
CFLAGS=$(MYCFLAGS)
LDFLAGS=-L$(MAKE_LIB) -I$(MAKE_INC)
LIBS=$(MYLIBS)

# DaoSCollection version.
V=11.0

# default, DaoLua
default: libDaoLibs.a libDaoQueue.a libDaoIconv.a libDaoJSON.a libDaoLua.a dsctools.so
	$(CC) $(CFLAGS) -o $(MAKE_BIN)/dsc src/main/main.c $(LDFLAGS) $(LIBS)
	$(CC) $(CFLAGS) -g -o $(MAKE_BIN)/dscG src/main/main.c $(LDFLAGS) $(LIBS)
	cp src/main/main.h $(MAKE_INC)/main.h -Rf
	@echo ""
	@echo ""
	@echo "MAKE SUCCESS!"

# install
install:
	if [ ! -x $(MAKE_BIN)/dsc ]; then echo "MUST MAKE!"; exit 1; fi
	if [ ! -x $(INSTALL_TOP) ]; then $(MKDIR) $(INSTALL_TOP); else rm $(INSTALL_TOP) -Rf; $(MKDIR) $(INSTALL_TOP); fi
	cp release/* $(INSTALL_TOP) -Rf
	@echo ""
	@echo ""
	@echo "INSTALL SUCCESS!"
	@echo "These are the parameters currently set in Makefile to install DaoSCollection $V!"
	@echo "INSTALL_TOP = $(INSTALL_TOP)"

# uninstall
uninstall:
	if [ -x $(INSTALL_TOP) ]; then rm $(INSTALL_TOP) -Rf; fi
	@echo ""
	@echo ""
	@echo "UNINSTALL SUCCESS!"

# clean
clean:
	cd src/Include/DaoLibs && $(MAKE) clean
	cd src/Include/DaoQueue && $(MAKE) clean
	cd src/Include/DaoIconv && $(MAKE) clean
	cd src/Include/DaoJSON && $(MAKE) clean
	cd src/Include/DaoLua && $(MAKE) clean
	cd $(MAKE_BIN) && $(RM) *
	cd $(MAKE_INC) && $(RM) *
	cd $(MAKE_LIB) && $(RM) *
	cd $(MAKE_LUALIB)/so && $(RM) *
	cd $(MAKE_BIN) && $(RM) *

libDaoLibs.a:
	cd src/Include/DaoLibs && $(MAKE)
	cp src/Include/DaoLibs/$@ $(MAKE_LIB)/$@ -Rf
	cp src/Include/DaoLibs/DaoLibs.h $(MAKE_INC)/DaoLibs.h -Rf
	
libDaoQueue.a:
	cd src/Include/DaoQueue && $(MAKE)
	cp src/Include/DaoQueue/$@ $(MAKE_LIB)/$@ -Rf
	cp src/Include/DaoQueue/DaoQueue.h $(MAKE_INC)/DaoQueue.h -Rf

libDaoIconv.a:
	cd src/Include/DaoIconv && $(MAKE)
	cp src/Include/DaoIconv/$@ $(MAKE_LIB)/$@ -Rf
	cp src/Include/DaoIconv/DaoIconv.h $(MAKE_INC)/DaoIconv.h -Rf

libDaoJSON.a:
	cd src/Include/DaoJSON && $(MAKE)
	cp src/Include/DaoJSON/$@ $(MAKE_LIB)/$@ -Rf
	cp src/Include/DaoJSON/cJSON.h $(MAKE_INC)/cJSON.h -Rf
	
libDaoLua.a:
	cd src/Include/DaoLua && $(MAKE)
	cp src/Include/DaoLua/$@ $(MAKE_LIB)/$@ -Rf
	cp src/Include/DaoLua/*.h $(MAKE_INC)/ -Rf

dsctools.so:
	$(CC) -shared -fPIC -o $(MAKE_LUALIB)/so/dsctools.so src/main/dsctools.c $(LDFLAGS) $(LIBS)
	cp src/main/dsctools.h $(MAKE_LUALIB)/so/dsctools.h -Rf

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: default install uninstall clean

# (end of Makefile)
