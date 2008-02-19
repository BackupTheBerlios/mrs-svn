# $Id: interpro.make,v 1.3 2003/02/07 08:44:13 dbman Exp $

DATABANK		= interpro
MRSLIBS			= interpro

include make.pre

DB_URL = ftp://ftp.ebi.ac.uk/pub/databases/interpro

# what to make

ZIPFILES = $(SRCDIR)interpro.xml.gz
#DATFILES = $(DSTDIR)interpro.xml

include make.post
