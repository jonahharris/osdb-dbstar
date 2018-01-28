##############################################################################
#
#       db.linux
#       Interactive Database Access (IDA) utility makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information ---------------------------------------------

APP = ida
DIR = ida

##############################################################################

include ../include/header.mak

CFLAGS += -DTASK_DEFN

##############################################################################
#   Application

exe: platdir $(APPS) install_bin

SRC = cnvfld.c curfcns.c dblist.c disprec.c ioctl.c editfcns.c listsel.c\
      main.c menu.c miscfcns.c mltfcns.c parfcns.c rdtext.c recfcns.c\
      setfcns.c tgetch.c topfcns.c tprintf.c
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB) $(PSPLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(TERMLIB) $(SYSLIBS)

##############################################################################

include ../include/footer.mak

