# $Id: prints.make,v 1.7 2003/03/13 07:31:05 hekkel Exp $

DATABANK		= prints
SRSLIBS			= prints

include make.pre

# Now determine what sequence files need to be generated

ZIPFILES = $(wildcard $(SRCDIR)prints*.dat.gz)
#DATFILES = $(DSTDIR)prints.dat

$(DATFILES): $(ZIPFILES)
	$(GZCAT)  $< > $@

include make.post
