##############################################################################
#
#       db.linux
#       Runtime makefile
#
#       Copyright (C) 1999 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information --------------------------------------------

LIB = libdbstar
DIR = runtime

##############################################################################

include ../include/header.mak

##############################################################################

.PRECIOUS: $(LIBS)

exe: platdir $(LIBS) install_lib

##############################################################################

SRC = alloc.c    clear.c    closeall.c connect.c  currency.c dberr.c\
      dblfcns.c  dbnum.c    dbstat.c   dbuserid.c delete.c   destroy.c\
      dio.c      discon.c   disdel.c   enter.c    fillnew.c  global.c\
      inifile.c  initenv.c  initial.c  inittab.c  internal.c ismember.c\
      isowner.c  keybuild.c keyfcns.c  keys.c     lcknfree.c libfcns.c\
      lockfcns.c lstat.c    makenew.c  mapchar.c  members.c  opens.c\
      ovfcns.c   pathfcns.c recfcns.c  recover.c  recs.c     renfile.c\
      rlbfcns.c  rwcurr.c   setdb.c    sets.c     settings.c taffcns.c\
      task.c     tmstamp.c  trlog.c    trxfcns.c  version.c

#############################################################################

## Objects

OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))
CFLAGS += -DTASK_DEFN

## Dependencies

$(OBJ): $(DBSTARINC)/psp.h $(DBSTARINC)/pspplat.h $(DBSTARINC)/db.star.h\
        $(DBSTARINC)/dbtype.h $(DBSTARINC)/dbxtrn.h $(DBSTARINC)/proto.h\
        $(DBSTARINC)/dproto.h

## Build rules

$(PLATFORM)/$(LIB).$(LIBSFX): $(OBJ)
	$(LD) $(LDFLAGS) $@ $(OBJ) $(THREADLIBS) $(PSPLIBLINK)

##############################################################################

include ../include/footer.mak

