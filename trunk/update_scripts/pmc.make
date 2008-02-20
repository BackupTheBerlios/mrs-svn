# $Id: pmc.make,v 1.7 2003/03/13 07:31:05 hekkel Exp $

DATABANK		= pmc
MRSLIBS			= pmc

include make.pre

DB_URL = ftp://ftp.ncbi.nlm.nih.gov/pub/pmc/
MIRROR_INCLUDE = articles.tar.gz

# Now determine what sequence files need to be generated

ZIPFILES = articles.tar.gz
DATFILES = 

#fetch:
#	rsync -av rsync://rsync.ncbi.nih.gov/pub/pmc/ $(SRCDIR)

data: 

include make.post

test:
	echo $(DATFILES)

