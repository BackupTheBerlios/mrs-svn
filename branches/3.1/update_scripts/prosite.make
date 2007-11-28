# $Id: prosite.make,v 1.6 2003/03/13 07:55:14 dbman Exp $

DATABANK		= prosite
MRSLIBS			= prosite prosite_doc

include make.pre

#DB_URL = ftp://ftp.expasy.org/databases/prosite/release/
DB_URL = ftp://ftp.ebi.ac.uk/pub/databases/prosite/
MIRROR_INCLUDE = prosite.*\.(dat|doc)$$

# Now determine what sequence files need to be generated
# 24-2-2003, the prosite.dat file changed name from .dat.gz to .dat at ebi

ZIPFILES = $(SRCDIR)prosite.dat $(SRCDIR)prosite.doc
#DATFILES = $(DSTDIR)prosite.dat $(DSTDIR)prosite.doc

include make.post
