##############################################################################
#
#       db.linux
#       Database Import Utility makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information ---------------------------------------------

APP = dbimp
DIR = dbimp

##############################################################################

include ../include/header.mak

CFLAGS += -DTASK_DEFN
YFLAGS += -d

##############################################################################
#   Application

exe: platdir $(APPS) install_bin

GENSRC = imp.c y.tab.h

SRC = asm_val.c dbexec.c dbfile.c dbimp.c init_key.c exfcns.c fld_move.c \
      imp.c lex.c trfcns.c dberr.c getnames.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

##############################################################################

include ../include/footer.mak

