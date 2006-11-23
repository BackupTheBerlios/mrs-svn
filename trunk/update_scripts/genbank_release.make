# $Id: genbank.make,v 1.1 2004/03/30 08:38:51 dbman Exp $

DATABANK		= genbank_release
MRSLIBS			= genbank_release
MRSSCRIPT		= genbank
MRS_PARTS       = 8

include make.pre

DB_URL	= ftp://ftp.ncbi.nih.gov/genbank/
MIRROR_INCLUDE = .*\.seq\.gz|GB_Release_Number|README\.genbank

# Now determine what sequence files need to be generated

ZIPFILES = $(wildcard $(SRCDIR)*.seq.gz)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

include make.post

test:
	echo $(ZIPFILES)
