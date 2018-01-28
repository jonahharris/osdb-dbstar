##############################################################################
#
#       db.linux
#       BOM example program makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Relative path to RDM home directory -----------------------------------

RELHOME  = ../..

## Define target information ---------------------------------------------

APP   = bom rbom
APPDB = bom rbom
DIR   = examples/bom

##############################################################################

include $(RELHOME)/include/header.mak

## Add to the standard INCLUDE ------------------------------

INCLUDE += -I$(PLATFORM)

##############################################################################
#   Application

exe: platdir $(APPDBS) $(APPS)

BOMSRC = bom.c
BOMOBJ = $(addprefix $(PLATFORM)/, $(BOMSRC:.c=.o))

$(PLATFORM)/bom: $(BOMOBJ) $(RTLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(BOMOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SYSLIBS)

RBOMSRC = rbom.c
RBOMOBJ = $(addprefix $(PLATFORM)/, $(RBOMSRC:.c=.o))

$(PLATFORM)/rbom: $(RBOMOBJ) $(RTLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(RBOMOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SYSLIBS)

##############################################################################

include $(RELHOME)/include/footer.mak

