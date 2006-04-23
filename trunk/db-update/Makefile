# Master Makefile for updating a set of databases

firstTarget: all

HOST = $(shell hostname -s)
include make_$(HOST).conf

ifeq ($(DATADIR),)
	please define the paths for your host system
endif

# Here are the databases that need to be updated.
# For each name listed here there should be a makefile
# with the same name and an .make extension and an entry in 
# the ftpmirror.cf file to mirror/fetch the data needed by this database

ALL_DATABASES	= $(basename $(wildcard *.make))

ifeq ($(DATABASES),)
DATABASES	= $(ALL_DATABASES)
endif

FAILED_DBS = $(sort $(shell cat $(STATUSDIR)failed_dbs.txt))
DATABASES	:= $(filter-out $(FAILED_DBS), $(DATABASES))

ifeq ($(NOT_DAILY),)
NOT_DAILY	= embl_release dbest dbsts genbank
endif

WEEKLY		= $(filter-out $(EXCLUDE_DBS), $(DATABASES))

DAILY		= $(filter-out $(NOT_DAILY), $(WEEKLY))

# internal rules and macros


ifneq ($(DAILY),)
DBDAILYFLAGS =	$(addprefix $(FLAGSDIR), $(DAILY))
endif

ifneq ($(WEEKLY),)
DBWEEKLYFLAGS =	$(addprefix $(FLAGSDIR), $(WEEKLY))
endif

DBALLFLAGS =	$(addprefix $(FLAGSDIR), $(ALL_DATABASES))

# The directories that need to exist in order to be able to run
# and the rules to create them if necessary.

DIRS = \
	$(SCRIPTDIR) \
	$(FLAGSDIR) \
	$(STATUSDIR)

${DIRS}:
	mkdir -p $@

dirs: $(DIRS)

# this may not run on all systems:
$(BINDIR)%: $(SCRIPTDIR)%.cpp
	g++ -o $@ $<

tools: $(BINDIR)run_and_log $(BINDIR)lock_and_run

# The meat of this makefile, the rule(s) to first fetch/mirror the data and
# then to execute the database's makefile.
# This rule also touches a flag in the status directory to be able to track
# the execution.

${DBALLFLAGS:=.make}:
	@ $(BINDIR)run_and_log -r 3 -l $(STATUSDIR)${@F:.make=}.make_log \
		$(MAKE) -f ${@F:.make=}.make data mrs blast || echo ${@F:.make=} >> $(STATUSDIR)failed_dbs.txt
	sync
	touch $(STATUSDIR)$(@F)
	touch $@

${DBALLFLAGS:=.fetched}:
	@ $(BINDIR)run_and_log -r 3 -l $(STATUSDIR)${@F:.fetched=}.mirror_log \
		$(MAKE) -f ${@F:.fetched=}.make fetch || echo ${@F:.make=} >> $(STATUSDIR)failed_dbs.txt
	sync
	touch $(STATUSDIR)$(@F)
	touch $@

${DBALLFLAGS:=.done}:
	@ $(MAKE) $(@:.done=.fetched)
	@ $(MAKE) $(@:.done=.make)
	touch $(STATUSDIR)$(@F)
	touch $@

# the main rules

daily: tools dirs ${DBDAILYFLAGS:=.done}
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Daily update done"

weekly: tools dirs ${DBWEEKLYFLAGS:=.done}
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Weekly update done"

all: tools dirs ${DBALLFLAGS:=.done}
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Complete update done"

alldailydata: tools dirs ${DBDAILYFLAGS:=.make}
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Complete remake done"

allweeklydata: tools dirs ${DBWEEKLYFLAGS:=.make}
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Complete remake done"

test:
	echo $(DATABASES)

${ALL_DATABASES}:
	$(MAKE) $(FLAGSDIR)$@.done
	rm $(FLAGSDIR)$@.*

# The next rules are used on cmbi8 to regenerate the structure databanks

dssp: uniprot pdb

hssp: dssp

pdbfinder: dssp hssp

pdbfinder2: pdbfinder

structure_dbs: pdbfinder2
	@ find $(FLAGSDIR) -type f | xargs rm
	echo "Structure DB update done"

