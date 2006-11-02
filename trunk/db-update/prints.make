# $Id: prints.make,v 1.7 2003/03/13 07:31:05 hekkel Exp $

DATABANK		= prints
MRSLIBS			= prints

include make.pre

DB_URL = ftp://ftp.ebi.ac.uk/pub/databases/prints/
MIRROR_INCLUDE = README|.*\.gz$$

# Now determine what sequence files need to be generated

ZIPFILES = $(wildcard $(SRCDIR)*.gz)
DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

PRINTS_VERSION_FETCHER =                   \
	while (<>)                             \
	{                                      \
		if (/PRINTS VERSION (\d+)\.(\d)/)  \
		{                                  \
			print "$$1_$$2";               \
			last;                          \
		}                                  \
	}

PRINTS_VERSION = $(shell $(GZCAT) $(SRCDIR)newpr.lis.gz | $(PERL) -e '$(PRINTS_VERSION_FETCHER)')

DATFILES := ${DATFILES:$(DSTDIR)prints$(PRINTS_VERSION)%=$(DSTDIR)prints%}

$(DSTDIR)prints%: $(SRCDIR)prints$(PRINTS_VERSION)%.gz
	$(GZCAT)  $< > $@

$(DSTDIR)newpr.lis: $(SRCDIR)newpr.lis.gz
	$(GZCAT)  $< > $@

data: $(DATFILES)

include make.post

test:
	echo $(DATFILES)

