# $Id: go.make,v 1.3 2004/03/30 08:38:06 dbman Exp $

DATABANK		= go
MRSLIBS			= go

include make.pre

# what to make

DB_URL = ftp://ftp.geneontology.org/pub/go/ontology/
MIRROR_INCLUDE = .*\.obo$$

ZIPFILES = $(SRCDIR)gene_ontology.obo
#DATFILES = $(DSTDIR)go.xml

VPATH += $(SRCDIR)archive

$(FLAGSDIR)%.gz.valid: %.gz
	$(GZCAT)  $< > /dev/null && touch $@

$(DATFILES): $(ZIPFILES)
	$(GZCAT)  $< > $@

include make.post
