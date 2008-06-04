# $Id: pdb.make,v 1.24 2004/12/12 08:33:46 maarten Exp $

DATABANK		= pdb
MRSLIBS			= pdb
MRS_PARTS		= 2

include make.pre

PRESERVE_DATE = 1

# Now determine what sequence files need to be generated
# another special set of rules

#DB_URL = ftp://ftp.rcsb.org//pub/pdb/data/structures/divided/pdb
#DB_URL = ftp://pdb.ccdc.cam.ac.uk//pdb/data/structures/divided/pdb
#MIRROR_INCLUDE = .*\.ent\.Z$$
#MIRROR_OPTIONS += -r

ZIPFILES = $(wildcard $(SRCDIR)*/*.gz)
DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# include separate dependancy files, generated with perl script
pdb.deps: $(ZIPFILES)
	$(PERL) generate_pdb_deps.pl pdb $(SRCDIR) $(DSTDIR)

cleanup_deps:
	rm -rf pdb.deps

fetch: cleanup_deps
	rsync -rltpvz --delete --port=33444 \
		rsync.rcsb.org::ftp_data/structures/divided/pdb/ \
		$(SRCDIR)

include pdb.deps

include make.post

test:
	echo $(ZIPFILES)
