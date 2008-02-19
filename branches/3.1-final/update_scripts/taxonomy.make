# $Id: taxon.make,v 1.3 2004/12/12 08:33:46 maarten Exp $

DATABANK		= taxonomy
SRSLIBS			= taxonomy
MRSLIBS			= taxonomy
MRS_DICT		= parent:text

include make.pre

#MRS_DICT_OPTIONS	:= -l 3

# Now determine what files need to be generated

DB_URL = ftp://ftp.ebi.ac.uk//pub/databases/taxonomy

ZIPFILES = $(SRCDIR)taxonomy.dat
#DATFILES = $(DSTDIR)taxonomy.dat

include make.post
