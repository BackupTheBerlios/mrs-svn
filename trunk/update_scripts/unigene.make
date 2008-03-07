# $Id: unigene.make,v 1.23 2004/12/12 08:33:46 maarten Exp $

DATABANK		= unigene
MRSLIBS			= unigene uniuniq # uniseq
MRS_PARTS		= 8

include make.pre

# Now determine what sequence files need to be generated

DB_URL = ftp://ftp.ncbi.nih.gov/repository/UniGene
MIRROR_INCLUDE = .*[^Z]$$
MIRROR_OPTIONS = -r

INFO_FILES = $(wildcard $(SRCDIR)*/*.info)
DATA_DIRS = $(foreach file, $(INFO_FILES), $(dir $(file)))

SECTIONS = $(notdir $(basename $(INFO_FILES)))
ZIPFILES = $(wildcard $(SRCDIR)*/*.seq.all.gz)

VPATH += $(addprefix :, $(subst  ,:, $(DATA_DIRS)))

#BLAST_FILES = $(addprefix $(BLASTDIR)uni_, $(addsuffix .formatted, $(SECTIONS)))

UNICLEAN =							\
	while (<>)						\
	{								\
		if (!/^\#/ && /.+/)			\
		{							\
			print $$_;				\
		}							\
	}

$(BLASTDIR)uni_%.formatted: $(SRCDIR)%.seq.all.gz
	$(GZCAT) $? | $(PERL) -e '$(UNICLEAN)' | $(FORMATDB) -pF -oT -sT -n $(basename $@) -i stdin
	touch $@

#blast: $(BLAST_FILES)
#	echo "TITLE "$(DATABANK) > $(BLASTDIR)/$(DATABANK).nal
#	echo "DBLIST "$(SECTIONS) >> $(BLASTDIR)/$(DATABANK).nal

include make.post
