# $Id: gpcrdb.make,v 1.1 2004/03/30 08:38:51 dbman Exp $

DATABANK		= gpcrdb
MRSLIBS			= gpcrdb
MRSSCRIPT		= uniprot
MRS_DICT        = de:kw:cc:ft:oc:og:ox:ref

include make.pre

DB_URL			= ftp://www.gpcr.org/pub/7tm/seq/
MIRROR_INCLUDE		= .*\.tar\.gz

# Now determine what sequence files need to be generated

ZIPFILES	= $(wildcard $(SRCDIR)*.gz)
DATFILES	= $(DSTDIR)gpcrdb.dat

$(DSTDIR)gpcrdb.dat: $(ZIPFILES)
	@ rm -rf $@
	cd $(DSTDIR); tar xOzf $(SRCDIR)seq.tar.gz > $@
	cd $(DSTDIR); tar xOzf $(SRCDIR)frag.tar.gz >> $@

include make.post
