# $Id: goa.make,v 1.4 2004/01/07 13:49:13 dbman Exp $

DATABANK		= goa
MRSLIBS			= goa

include make.pre

DB_URL = ftp://ftp.ebi.ac.uk//pub/databases/GO/goa/UNIPROT
MIRROR_INCLUDE = gene_association\.goa_uniprot\.gz$$
MIRROR_OPTIONS += -r

# what to make

ZIPFILES = gene_association.goa_uniprot.gz
#DATFILES = $(addprefix $(DSTDIR), $(basename $(notdir $(ZIPFILES))))

include make.post
