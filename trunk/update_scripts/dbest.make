# $Id: dbest.make,v 1.4 2004/12/12 08:33:45 maarten Exp $

DATABANK		= dbest
SRSLIBS			= dbest
MRSLIBS			= dbest
MRS_PARTS		= 8

include make.pre

# what to make

DB_URL = ftp://ftp.ncbi.nih.gov//repository/dbEST/gzipped
MIRROR_INCLUDE = dbEST.*reports\..*gz$$

ZIPFILES = $(wildcard $(SRCDIR)*.gz)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

include make.post
