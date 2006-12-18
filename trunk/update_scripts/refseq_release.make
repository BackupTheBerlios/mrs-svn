# $Id: refseq.make,v 1.9 2004/04/02 07:13:24 dbman Exp $

DATABANK		= refseq_release
MRSLIBS			= refseq_release
MRSSCRIPT		= genbank
MRS_DICT		= comment:features:keywords:organism:reference
MRS_PARTS		= 2

include make.pre

# what to make

DB_URL = ftp://ftp.ncbi.nih.gov//refseq/release/complete
MIRROR_INCLUDE = .*\.(gbff|gpff)\.gz$$

ZIPFILES = $(wildcard $(SRCDIR)*.gz)
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# BLAST_FILES	= $(BLASTDIR)refseqp.formatted $(BLASTDIR)refseqn.formatted

$(BLASTDIR)refseqp.formatted: $(wildcard $(SRCDIR)*.gpff.gz)
	$(GZCAT) $? | $(BINDIR)genbank2fasta -d $(DATABANK) | $(FORMATDB) -pT -oT -sT -n $(basename $@) -i stdin
	touch $@
	
$(BLASTDIR)refseqn.formatted: $(wildcard $(SRCDIR)*.gbff.gz)
	$(GZCAT) $? | $(BINDIR)genbank2fasta -d $(DATABANK) | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin
	touch $@

# main rules

include make.post
