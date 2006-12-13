# $Id: emnew.make,v 1.39 2004/03/30 08:38:06 dbman Exp $

DATABANK		= embl_updates
MRSLIBS			= embl_updates
MRSSCRIPT		= embl
MRS_PARTS		= 4

include make.pre

# what to make

DB_URL			= ftp://ftp.ebi.ac.uk//pub/databases/embl/new
MIRROR_INCLUDE = r\d+u\d+\.dat\.gz$$

ZIPFILES = $(notdir $(wildcard $(SRCDIR)*.dat.gz))
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# BLAST!
# What sections do we have and what files need to be generated:

SECTIONS = $(sort $(shell $(BASH) -c "echo $(ZIPFILES) | sed 's/[0-9]*\.dat\.gz//g'"))
SECTIONS := $(addprefix em_, $(filter-out cum_ relnotes.txt, $(SECTIONS)) cum)

NSQFILES = $(addprefix $(BLASTDIR), $(addsuffix .nsq, $(SECTIONS)))

# Create the blast indices using $(FORMATDB)

$(BLASTDIR)em_cum.nsq: $(wildcard $(SRCDIR)cum_?.dat.gz)
	$(GZCAT) $? | $(BINDIR)swiss2fasta -d $(DATABANK) | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin

$(BLASTDIR)%.nsq: $(ZIPFILES)
	$(GZCAT) $(wildcard $(SRCDIR)${@:$(BLASTDIR)em_%.nsq=%}*.dat.gz) | \
		$(BINDIR)swiss2fasta -d $(DATABANK) | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin
	
#blast:  $(NSQFILES)
#	echo "TITLE "$(DATABANK) > $(BLASTDIR)/$(DATABANK).nal
#	echo "DBLIST "$(SECTIONS) >> $(BLASTDIR)/$(DATABANK).nal

include make.post
