# $Id: refseq.make,v 1.9 2004/04/02 07:13:24 dbman Exp $

DATABANK		= refseq_updates
MRSLIBS			= refseq_updates
MRSSCRIPT		= genbank
MRS_UPDATE		= 1
MRS_DICT		= comment:features:keywords:organism:reference
MRS_MERGE_LIBS	= refseq
MRS_PARTS		= 2

include make.pre

# what to make

DB_URL = ftp://ftp.ncbi.nih.gov//refseq/daily/
MIRROR_INCLUDE = rsnc\..*\.(gbff|gpff)\.gz$$

ZIPFILES = $(wildcard $(SRCDIR)*.gz)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# main rules

include make.post
