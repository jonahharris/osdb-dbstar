##############################################################################
#
#       db.linux
#       Utility makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

## Define directory/application information ------------------------------

APP = console datdump dbcheck dbclrlb dbexp dchain ddlp initdb keybuild\
      keydump keypack lmclear lmping prdbd
DIR = utility

##############################################################################

include ../include/header.mak

CFLAGS += -DTASK_DEFN -I$(RELHOME)/ida
YFLAGS += -d

##############################################################################

exe: platdir $(APPS) install_bin

##############################################################################

GENSRC = ddlp.c ddlp.h

CONSOLESRC = console.c display.c tables.c ioctl.c tprintf.c
CONSOLEOBJ = $(addprefix $(PLATFORM)/, $(CONSOLESRC:.c=.o))

$(PLATFORM)/console: $(CONSOLEOBJ) $(PSPLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(CONSOLEOBJ) $(PSPLIBLINK) $(SYSLIBS) $(TERMLIB)

$(PLATFORM)/cnvfld.o: cnvfld.c
	$(CC) -c $(CCONLY) $(CFLAGS) cnvfld.c -o $@

DBCHECKSRC = dbcheck.c dbcache.c cnvfld.c
DBCHECKOBJ = $(addprefix $(PLATFORM)/, $(DBCHECKSRC:.c=.o))

$(PLATFORM)/dbcheck: $(DBCHECKOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DBCHECKOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

DCHAINSRC = dchain.c 
DCHAINOBJ = $(addprefix $(PLATFORM)/, $(DCHAINSRC:.c=.o))

$(PLATFORM)/dchain: $(DCHAINOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DCHAINOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

DBCLRLBSRC = dbclrlb.c 
DBCLRLBOBJ = $(addprefix $(PLATFORM)/, $(DBCLRLBSRC:.c=.o))

$(PLATFORM)/dbclrlb: $(DBCLRLBOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DBCLRLBOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

KEYBUILDSRC = keybuild.c 
KEYBUILDOBJ = $(addprefix $(PLATFORM)/, $(KEYBUILDSRC:.c=.o))

$(PLATFORM)/keybuild: $(KEYBUILDOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(KEYBUILDOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

KEYPACKSRC = keypack.c packfile.c 
KEYPACKOBJ = $(addprefix $(PLATFORM)/, $(KEYPACKSRC:.c=.o))

$(PLATFORM)/keypack: $(KEYPACKOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(KEYPACKOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

LMCLEARSRC = lmclear.c
LMCLEAROBJ = $(addprefix $(PLATFORM)/, $(LMCLEARSRC:.c=.o))

$(PLATFORM)/lmclear: $(LMCLEAROBJ) $(PSPLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(LMCLEAROBJ) $(PSPLIBLINK) $(SYSLIBS)

LMPINGSRC = lmping.c
LMPINGOBJ = $(addprefix $(PLATFORM)/, $(LMPINGSRC:.c=.o))

$(PLATFORM)/lmping: $(LMPINGOBJ) $(PSPLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(LMPINGOBJ) $(PSPLIBLINK) $(SYSLIBS)


DDLPSRC := ddlp.c ddlmain.c ddltable.c ddllex.c ddlxref.c ddlalign.c
DDLPOBJ := $(addprefix $(PLATFORM)/, $(DDLPSRC:.c=.o)) 

$(PLATFORM)/ddlp: $(DDLPOBJ) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DDLPOBJ) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

INITDBSRC = initdb.c
INITDBOBJ = $(addprefix $(PLATFORM)/, $(INITDBSRC:.c=.o)) 

$(PLATFORM)/initdb: $(INITDBOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(INITDBOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

PRDBDSRC = prdbd.c
PRDBDOBJ = $(addprefix $(PLATFORM)/, $(PRDBDSRC:.c=.o)) 

$(PLATFORM)/prdbd: $(PRDBDOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(PRDBDOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

KEYDUMPSRC = keydump.c cnvfld.c
KEYDUMPOBJ = $(addprefix $(PLATFORM)/, $(KEYDUMPSRC:.c=.o))

$(PLATFORM)/keydump: $(KEYDUMPOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(KEYDUMPOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

DATDUMPSRC = datdump.c cnvfld.c
DATDUMPOBJ = $(addprefix $(PLATFORM)/, $(DATDUMPSRC:.c=.o)) 

$(PLATFORM)/datdump: $(DATDUMPOBJ)  $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DATDUMPOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

DBEXPSRC = dbexp.c pr_fcns.c
DBEXPOBJ = $(addprefix $(PLATFORM)/, $(DBEXPSRC:.c=.o)) 

$(PLATFORM)/dbexp: $(DBEXPOBJ) $(RTLIB) $(PSPLIB) $(SGLIB)
	$(LDBIN) $(LDBINFLAGS) $@ $(DBEXPOBJ) $(RTLIBLINK) $(PSPLIBLINK) $(SGLIBLINK) $(SYSLIBS)

##############################################################################

include ../include/footer.mak

