DATABANK		= structure_factors

include make.pre

NO_VALIDATE = 1
PRESERVE_DATE = 1

# Now determine what sequence files need to be generated
# another special set of rules

#DB_URL = ftp://pdb.ccdc.cam.ac.uk//pdb/data/structures/divided/structure_factors
#MIRROR_INCLUDE = .*\.ent\.Z$$
#MIRROR_OPTIONS += -r

ZIPFILES = $(wildcard $(SRCDIR)/??/r*sf.ent.Z)
DATFILES = $(addprefix $(DSTDIR), $(notdir $(basename $(ZIPFILES))))

# include separate dependancy files, generated with perl script
sf.deps: $(ZIPFILES)
	$(PERL) generate_pdb_deps.pl sf $(SRCDIR) $(DSTDIR)

cleanup_deps:
	rm -rf sf.deps

fetch: cleanup_deps
	rsync -rltpvz --delete --port=33444 \
		rsync.rcsb.org::ftp_data/structures/divided/structure_factors/ \
		$(SRCDIR)

include sf.deps

include make.post

test:
	echo $(ZIPFILES)
