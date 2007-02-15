# $Id: enzyme.make,v 1.4 2002/11/29 14:34:54 dbman Exp $

DATABANK		= enzyme
MRSLIBS			= enzyme

include make.pre

# Now determine what sequence files need to be generated

DB_URL = ftp://ftp.ebi.ac.uk/pub/databases/enzyme/release_with_updates

ZIPFILES = $(SRCDIR)enzyme.dat

include make.post
