# $Id: sp_tr_nrdb.make,v 1.21 2004/02/18 13:24:22 dbman Exp $

DATABANK			= ipi
MRSLIBS				= ipi_human ipi_mouse ipi_rat ipi_zebrafish ipi_arabidopsis ipi_chicken ipi_cow
MRSSCRIPT			= uniprot

include make.pre

# Now determine what files need to be generated

DB_URL			= ftp://ftp.ebi.ac.uk/pub/databases/IPI/current/

ZIPFILES = $(wildcard $(SRCDIR)ipi.*.dat.gz)

include make.post

test:
	echo $(ZIPFILES)
