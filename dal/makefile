##############################################################################
#
#       db.linux
#       DAL makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information ---------------------------------------------

APP 		= dal
DIR 		= dal

##############################################################################

include ../include/header.mak

CFLAGS += -DTASK_DEFN
YFLAGS += -d

##############################################################################
#   Application

exe: platdir $(APPS) install_bin

GENSRC = dal.c y.tab.h

SRC = dal.c dalcurr.c dalexec.c dalfcns.c dalio.c dallex.c dalmain.c\
      dalschem.c dalvar.c dalwhile.c getnames.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

##############################################################################

include ../include/footer.mak

