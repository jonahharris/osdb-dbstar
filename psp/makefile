##############################################################################
#
#       db.linux
#       Program Support Package makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define target information --------------------------------------------

LIB         = libpsp
DIR         = psp

##############################################################################

include ../include/header.mak

##############################################################################

.PRECIOUS: $(LIBS)

exe: platdir $(LIBS) install_lib

##############################################################################

SRC = psp.c pspfile.c pspmem.c psplmc.c pspposix.c psplinux.c psptcpux.c

#############################################################################

## Objects

OBJ = $(addprefix $(PLATFORM)/, $(SRC:.c=.o))

## Dependencies

$(OBJ): $(DBSTARINC)/psp.h $(DBSTARINC)/pspplat.h ./pspint.h

## Build rules

$(PLATFORM)/libpsp.$(LIBSFX): $(OBJ)
	$(LD) $(LDFLAGS) $@ $(OBJ) $(THREADLIBS)

##############################################################################

include ../include/footer.mak

