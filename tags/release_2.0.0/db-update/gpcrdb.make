# $Id: gpcrdb.make,v 1.1 2004/03/30 08:38:51 dbman Exp $

DATABANK		= gpcrdb

# not in SRS
SRSLIBS			= 

include make.pre

# Now determine what sequence files need to be generated

ZIPFILES	= $(SRCDIR)$(shell $(BASH) -c "ls -rt1 $(SRCDIR) | tail -1")
DATFILES	= $(DSTDIR)gpcrdb.dat
FASTAFILES	= $(FASTADIR)gpcrdb.fa

$(DSTDIR)gpcrdb.dat: $(ZIPFILES)
	@ rm -rf $@
	find $(SRCDIR) -type f | xargs cat >> $@

$(FASTADIR)%.fa: $(DSTDIR)%.dat
	$(BINDIR)sreformat --informat embl fasta $< > $@

fasta: $(FASTAFILES)

#blast: $(FASTAFILES)
#	$(FORMATDB) -pT -oT -sT -n $(BLASTDIR)gpcrdb $(FASTAFILES)

include make.post
