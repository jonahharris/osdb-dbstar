sysdir:
	@-sh -c "if [ ! -d $(PLATFORM) ] ; then mkdir $(PLATFORM) ; else exit 0; fi"

platdir: sysdir 

bindir:
	@-sh -c "if [ ! -d $(RELHOME)/bin ] ; then mkdir $(RELHOME)/bin ; else exit 0; fi"

libdir:
	@-sh -c "if [ ! -d $(RELHOME)/lib ] ; then mkdir $(RELHOME)/lib ; else exit 0; fi"

$(RELHOME)/bin/%: 
	@-sh -c "if [ ! -f $@ ] ; then ln -fs ../$(DIR)/$(PLATFORM)/$(@F) $(@D) ; else exit 0; fi"
	
install_bin: bindir $(addprefix $(RELHOME)/bin/, $(APP))

$(RELHOME)/lib/%: 
	@-sh -c "if [ ! -f $@ ] ; then ln -fs ../$(DIR)/$(PLATFORM)/$(@F).$(LIBSFX) $(@D) ; else exit 0; fi"

install_lib: libdir $(addprefix $(RELHOME)/lib/, $(LIB))

tidy:
	@-rm -f core .pure .ctag* .make* *.err *.dbl so_lo* *.taf *.log *.lfg
	@-rm -f dbimp.kfl

clean: tidy
	@-rm -rf $(PLATFORM) 

realclean: clean
	@-rm -f *% *.bak

cleanlib:
ifdef LIB
	@-rm -f $(LIB)
else
	
endif

cleanbin:
ifdef BIN
	@-rm -f $(APP)
else
	
endif

.SECONDARY: $(GENSRC)
