# $Id: blocks.make,v 1.5 2004/01/07 13:49:13 dbman Exp $

DATABANK		= blocks
SRSLIBS			= blocks

include make.pre

# Now determine what sequence files need to be generated

ZIPFILES = $(SRCDIR)blocks.tar.gz
#DATFILES = $(DSTDIR).unpacked_blocks
PRESERVE_DAT_FILES = 1

$(DSTDIR).unpacked_blocks: $(SRCDIR)blocks.tar.gz
	cd $(@D); gunzip -c $< | tar xf -
	touch $@

include make.post

