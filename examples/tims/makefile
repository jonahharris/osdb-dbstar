##############################################################################
#
#       db.linux
#       TIMS example program makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Relative path to RDM home directory -----------------------------------

RELHOME  = ../..

## Define target information ---------------------------------------------

APP   = tims
APPDB = tims
DIR   = examples/tims

##############################################################################

include $(RELHOME)/include/header.mak

## Add to the standard INCLUDE ------------------------------

INCLUDE += -I$(PLATFORM)

##############################################################################
#   Application

exe: platdir $(APPDBS) $(APPS)

SRC = borrow.c delinfo.c entinfo.c findpubs.c listauth.c listkeys.c tims.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SYSLIBS)

##############################################################################

include $(RELHOME)/include/footer.mak

