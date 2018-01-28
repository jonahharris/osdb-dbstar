##############################################################################
#
#       db.linux
#       Master makefile
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

all:
	cd psp; $(MAKE)
	cd runtime; $(MAKE)
	cd utility; $(MAKE)
	cd dal; $(MAKE)
	cd dbedit; $(MAKE)
	cd dbimp; $(MAKE)
	cd ida; $(MAKE)
	cd lm; $(MAKE)
	cd examples/tims; $(MAKE)
	cd examples/sales; $(MAKE)
	cd examples/bom; $(MAKE)

tidy cleanbin cleanlib:
	cd psp; $(MAKE) $@
	cd runtime; $(MAKE) $@
	cd utility; $(MAKE) $@
	cd dal; $(MAKE) $@
	cd dbedit; $(MAKE) $@
	cd dbimp; $(MAKE) $@
	cd ida; $(MAKE) $@
	cd lm; $(MAKE) $@
	cd examples/tims; $(MAKE) $@
	cd examples/sales; $(MAKE) $@
	cd examples/bom; $(MAKE) $@

clean realclean:
	cd psp; $(MAKE) $@
	cd runtime; $(MAKE) $@
	cd utility; $(MAKE) $@
	cd dal; $(MAKE) $@
	cd dbedit; $(MAKE) $@
	cd dbimp; $(MAKE) $@
	cd ida; $(MAKE) $@
	cd lm; $(MAKE) $@
	cd examples/tims; $(MAKE) $@
	cd examples/sales; $(MAKE) $@
	cd examples/bom; $(MAKE) $@
	-rm -rf bin lib
