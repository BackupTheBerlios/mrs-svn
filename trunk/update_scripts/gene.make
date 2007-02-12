# $Id: gene.make,v 1.4 2004/01/07 13:49:13 dbman Exp $

DATABANK		= gene
MRSLIBS			= gene

include make.pre

DB_URL = ftp://ftp.ncbi.nlm.nih.gov/gene/DATA/ASN_BINARY
MIRROR_INCLUDE=All_Data\.ags\.gz

# what to make

ZIPFILES = $(SRCDIR)All_Data.ags.gz
# DATFILES = $(DSTDIR)All_Data.xgs.gz

$(DSTDIR)All_Data.xgs.gz: $(SRCDIR)All_Data.ags.gz
	$(GENE2XML) -i $< -o stdout -b -c | gzip -c > $@

include make.post
