##############################################################################
#
#       db.linux
#       Lock Manager makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information ---------------------------------------------

APP = lm
DIR = lm

##############################################################################

include ../include/header.mak

##############################################################################
#   Application

exe: platdir $(APPS) install_bin

SRC = lm.c lmcore.c lm_fcns.c fileid.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(PSPLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(PSPLIBLINK) $(SYSLIBS)

##############################################################################

include ../include/footer.mak

