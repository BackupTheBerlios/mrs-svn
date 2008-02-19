# $Id: prints.make,v 1.7 2003/03/13 07:31:05 hekkel Exp $

DATABANK		= sifts
MRSLIBS			= sifts

include make.pre

DB_URL = ftp://ftp.ebi.ac.uk/pub/databases/msd/sifts/xml/
MIRROR_INCLUDE = README|.*\.gz$$

# Now determine what sequence files need to be generated

ZIPFILES = $(wildcard $(SRCDIR)*.gz)
DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

data: $(DATFILES)

include make.post

test:
	echo $(DATFILES)

