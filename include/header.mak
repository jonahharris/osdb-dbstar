##############################################################################
#
#       db.linux
#       Makefile headers
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Relative path to db.* home directory -----------------------------------
##  May be overridden by makefiles

ifndef RELHOME
RELHOME = ..
endif

## Compilation mode (optimize/debug) -------------------------------------
##  Optimization will on occasion cause problems - if so, TURN IT OFF

ifdef DEBUG
OPTIMIZE    =
##DEBUG       = -g -DDB_DEBUG
DEBUG       = -g
else
OPTIMIZE    =
DEBUG       =
endif

## Common build variables ---------------------------------------------

AR          = ar
ARFLAGS     = rcv

# All header files should reside in the current directory, or in ------
#   the common header file directory.

DBSTARINC = $(RELHOME)/include
INCLUDE   = -I$(DBSTARINC)

## Compilation/linking options -----------------------------------------

CFLAGS          = $(DEBUG) $(OPTIMIZE) $(INCLUDE)
LDBINFLAGS      = $(DEBUG)

PLATFORM   = lnx
PLATDEF    = LINUX
CC         = gcc
CFLAGS     += -W -Wimplicit -Wswitch -Wcomment -Wformat -DUNIX -DLINUX -I/usr/include
LDBIN      = gcc
LDBINFLAGS += -o
LD         = gcc
LDFLAGS    += $(DEBUG) -shared -o
SYSLIBS    +=
LIBSFX     = so
TERMLIB    += -lncurses
THREADLIBS = -lpthread

ifdef SAFEGARDE
endif

##############################################################################
#   db.* libraries

RTSRC  = $(RELHOME)/runtime
RTLDIR = $(RELHOME)/runtime/$(PLATFORM)
PSPDIR = $(RELHOME)/psp/$(PLATFORM)

RTLIB  = $(RTLDIR)/libdbstar.$(LIBSFX)
PSPLIB = $(PSPDIR)/libpsp.$(LIBSFX)

RTLIBLINK  = -L$(RTLDIR) -ldbstar
PSPLIBLINK = -L$(PSPDIR) -lpsp

ifdef SAFEGARDE
CFLAGS    += -DSAFEGARDE
SGDIR     = $(RELHOME)/sg/$(PLATFORM)
SGLIB     = $(SGDIR)/libsg.$(LIBSFX)
SGLIBLINK = -L$(SGDIR) -lsg
else
SGLIB     =
SGLIBLINK =
endif
##############################################################################
#   Target variables

APPS    = $(addprefix $(PLATFORM)/, $(APP))
APPDBS  = $(addsuffix .dbd, $(addprefix $(PLATFORM)/, $(APPDB)))
LIBS    = $(addsuffix .$(LIBSFX), $(addprefix $(PLATFORM)/, $(LIB)))

##############################################################################
#   Default compilation rules

$(PLATFORM)/%.o: %.c
	$(CC) $(CCONLY) $(CFLAGS) -c $< -o $@

$(PLATFORM)/%.o: $(RTSRC)/%.c
	$(CC) $(CCONLY) $(CFLAGS) -DNOLIB -c $< -o $@

$(PLATFORM)/%.o: ../dal/%.c
	$(CC) $(CCONLY) $(CFLAGS) -c $< -o $@

$(PLATFORM)/%.o: ../ida/%.c
	$(CC) $(CCONLY) $(CFLAGS) -c $< -o $@

$(PLATFORM)/%.dbd: %.ddl
	cd $(PLATFORM); ddlp $(DDLPOPTS) ../$<
	cd $(PLATFORM); initdb -y $(basename $<)
	-sh -c "if [ -f $(basename $<).imp ] ; then \
                sed -e 's/foreach \"/foreach \"..\//' $(basename $<).imp > $(PLATFORM)/$(basename $<).imp; \
                cd $(PLATFORM); \
                ../$(RELHOME)/dbimp/$(PLATFORM)/dbimp -n $(basename $<).imp; \
        else \
                exit 0; \
        fi"



