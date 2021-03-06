# $Id: sp_tr_nrdb.make,v 1.21 2004/02/18 13:24:22 dbman Exp $

DATABANK			= uniprot
MRSLIBS				= sprot trembl
MRS_MERGE_LIBS		= uniprot
MRSSCRIPT			= uniprot

include make.pre

# Now determine what files need to be generated

DB_URL			= ftp://ftp.ebi.ac.uk//pub/databases/uniprot/knowledgebase/
MIRROR_INCLUDE = .*\.dat\.gz$$|reldate\.txt

ZIPFILES = $(SRCDIR)uniprot_sprot.dat.gz $(SRCDIR)uniprot_trembl.dat.gz

$(BLASTDIR)%.psq: $(SRCDIR)uniprot_%.dat.gz
	$(GZCAT) $? | $(SWISS_TO_FASTA) -d ${@F:.psq=} | $(FORMATDB) -pT -oT -sT -n $(basename $@) -i stdin

BLAST_FILES = $(BLASTDIR)sprot.psq $(BLASTDIR)trembl.psq

include make.post
