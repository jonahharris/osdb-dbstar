##############################################################################
#
#       db.linux
#       Database Editor makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information ---------------------------------------------

APP = dbedit
DIR = dbedit

##############################################################################

include ../include/header.mak

CFLAGS += -DTASK_DEFN

##############################################################################
#   Application

exe: platdir $(APPS) install_bin

SRC = cnvfld.c dbe_comm.c dbe_disp.c dbe_edit.c dbe_edx.c dbe_err.c\
      dbe_goto.c dbe_help.c dbe_init.c dbe_io.c dbe_main.c dbe_misc.c\
      dbe_show.c dbe_str.c dbe_vrfy.c 
OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

$(APPS): $(OBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(OBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

$(PLATFORM)/cnvfld.o: ../utility/cnvfld.c
	$(CC) -DDBEDIT $(CCONLY) $(CFLAGS) -c $< -o $@

##############################################################################

include ../include/footer.mak

