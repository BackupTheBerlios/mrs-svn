# $Id: humrep.make,v 1.12 2004/12/12 08:33:45 maarten Exp $

DATABANK		= alu

# not in SRS anymore
SRSLIBS			= 

include make.pre

# Now determine what sequence files need to be generated

DB_URL			= ftp://dbman:DBman2@t2.teras.sara.nl//bioasp/data1/db/raw/alu/
#EMIRROR_OPTIONS += --recursive

ZIPFILES =	$(wildcard $(SRCDIR)humrep.ref.gz)
#DATFILES =	$(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

FASTAFILES = $(FASTADIR)humrep.fa

HUMREP_TO_FASTA = \
	while (<>)						\
	{								\
		if (/^>DL;(.+)/)			\
		{							\
			print ">humrep:$$1 ";	\
		}							\
		else						\
		{							\
			print $$_;				\
		}							\
	}

$(FASTADIR)%.fa: $(SRCDIR)%.seq
	$(PERL) -e '$(HUMREP_TO_FASTA)' < $< > $@

# NOTE!!! Don't use $(FORMATDB) on this one, too few entries and no use

#blast: $(FASTAFILES)
#	$(FORMATDB) -pF -oT -sT -n $(BLASTDIR)humrep -i $(FASTAFILES)

# the main rules

include make.post
