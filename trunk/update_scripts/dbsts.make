# $Id: dbsts.make,v 1.3 2003/03/13 07:31:04 hekkel Exp $

DATABANK		= dbsts
SRSLIBS			= dbsts

include make.pre

# what to make

ZIPFILES = $(wildcard $(SRCDIR)*.Z)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

include make.post
