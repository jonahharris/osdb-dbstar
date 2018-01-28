##############################################################################
#
#       db.linux
#       Sales and Inventory example makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Relative path to db.* home directory -----------------------------------

RELHOME	= ../..

## Define target information ---------------------------------------------

APP   = exupd
APPDB = sales invntory
DIR   = examples/sales

##############################################################################

include $(RELHOME)/include/header.mak

## Add to the standard INCLUDE ------------------------------

INCLUDE += -I$(PLATFORM)

##############################################################################
#   Application

exe: platdir $(APPDBS) $(APPS) $(PLATFORM)/managers.dal

SRC = exupd.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(SYSLIBS)

$(PLATFORM)/managers.dal: $(PLATFORM)/sales.dbd
	sed -e 's/salemgrs/..\/salemgrs/' managers.dal > $(PLATFORM)/managers.dal
	cp notes.asc $(PLATFORM)
	cd $(PLATFORM); ../$(RELHOME)/dal/$(PLATFORM)/dal managers.dal
	cd $(PLATFORM); ./$(APP)

##############################################################################

include $(RELHOME)/include/footer.mak

