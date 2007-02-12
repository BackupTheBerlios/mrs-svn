# $Id: prodom.make,v 1.7 2003/03/13 07:31:05 hekkel Exp $

DATABANK		= prodom
SRSLIBS			= prodom

include make.pre

# Now determine what sequence files need to be generated

ZIPFILES = $(wildcard $(SRCDIR)prodom*.srs.gz)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

include make.post
