# $Id: emnew.make,v 1.39 2004/03/30 08:38:06 dbman Exp $

DATABANK		= embl_release
MRSLIBS			= embl_release
MRSSCRIPT		= embl
MRS_PARTS		= 8
MRS_DICT        = de:cc:ft:kw:oc:og:os:ref
#MRS_DICT_OPTIONS	= -n 10 -l 4 -D

include make.pre

# what to make

DB_URL			= ftp://ftp.ebi.ac.uk//pub/databases/embl/release
MIRROR_INCLUDE = .*\.dat\.gz$$|relnotes\.txt

ZIPFILES = $(notdir $(wildcard $(SRCDIR)*.dat.gz))
#DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# BLAST!
# What sections do we have and what files need to be generated:

SECTIONS = $(sort $(shell $(BASH) -c "echo $(notdir $(ZIPFILES)) | sed 's/[0-9]*\.dat\.gz//g'"))
SECTIONS := $(addprefix em_, $(filter-out gss% htg% wgs% relnotes.txt, $(SECTIONS)) gss htg htgo wgs)

# BLAST_FILES = $(addprefix $(BLASTDIR), $(addsuffix .formatted, $(SECTIONS)))

# Create the blast indices using $(FORMATDB)

$(BLASTDIR)em_htg.formatted: $(wildcard $(SRCDIR)htg_*.dat.gz)
	$(GZCAT) $? | $(BINDIR)swiss2fasta -d $(DATABANK) | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin
	touch $@

$(BLASTDIR)%.formatted: $(ZIPFILES)
	$(GZCAT) $(wildcard $(SRCDIR)${@:$(BLASTDIR)em_%.formatted=%}*.dat.gz) | \
		$(BINDIR)swiss2fasta -d $(DATABANK) | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin
	touch $@
	
# Put all together

#blast: $(BLAST_FILES)
#	echo "TITLE "$(DATABANK) > $(BLASTDIR)/$(DATABANK).nal
#	echo "DBLIST "$(SECTIONS) >> $(BLASTDIR)/$(DATABANK).nal

include make.post
