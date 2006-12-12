# $Id: locuslink.make,v 1.4 2004/12/12 08:33:45 maarten Exp $

DATABANK		= locuslink
SRSLIBS			= locuslink
MRSLIBS			= locuslink

include make.pre

# what to make

DB_URL = ftp://ftp.ncbi.nih.gov/refseq/LocusLink/ARCHIVE
MIRROR_INCLUDE = LL_tmpl.*

ZIPFILES = $(SRCDIR)LL_tmpl.gz
#DATFILES = $(DSTDIR)LL_tmpl

include make.post
