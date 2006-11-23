# $Id: pfam.make,v 1.6 2004/12/12 08:33:46 maarten Exp $

DATABANK		= pfam
MRSLIBS			= pfama pfamb pfamseed
MRSSCRIPT		= pfam

include make.pre

# Now determine what sequence files need to be generated

DB_URL = ftp://ftp.sanger.ac.uk/pub/databases/Pfam/current_release/

ZIPFILES =	$(SRCDIR)Pfam-A.full.gz \
			$(SRCDIR)Pfam-B.gz \
			$(SRCDIR)Pfam-A.seed.gz \
			$(SRCDIR)swisspfam.gz \
			$(SRCDIR)Pfam_fs.gz \
			$(SRCDIR)Pfam_ls.gz

#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

include make.post
