# $Id: ligand.make,v 1.5 2004/12/12 08:33:45 maarten Exp $

DATABANK		= ligand
SRSLIBS			= lenzyme lcompound
MRSLIBS			= ligand-enzyme ligand-compound ligand-glycan ligand-reaction
MRSSCRIPT		= ligand

include make.pre

# Now determine what sequence files need to be generated

DB_URL = ftp://ftp.genome.ad.jp//pub/kegg/ligand
MIRROR_INCLUDE = .*(compound|enzyme|glycan|reaction)$$

ZIPFILES = $(SRCDIR)compound $(SRCDIR)enzyme
#DATFILES = $(DSTDIR)compound $(DSTDIR)enzyme

include make.post
