# $Id: omim.make,v 1.6 2004/12/12 08:33:46 maarten Exp $

DATABANK		= omim
MRSLIBS			= omim mimmap

include make.pre

# Now determine what sequence files need to be generated

DB_URL = ftp://ftp.ncbi.nih.gov/repository/OMIM

ZIPFILES = $(SRCDIR)omim.txt.Z $(SRCDIR)genemap 
DATFILES = $(DSTDIR)omim.txt $(DSTDIR)mimmap.txt 

mrs: data

$(DSTDIR)mimmap.txt: $(SRCDIR)genemap
	awk -f $(SCRIPTDIR)genemap.awk $< > $@
#	awk -f $(SCRIPTDIR)genemap.awk $< | $(BINDIR)wordwrap 10 80 > $@

include make.post
