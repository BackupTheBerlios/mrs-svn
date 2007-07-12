DATABANK		= rebase
MRSLIBS			= rebase rebase_commdata

include make.pre

DB_URL = ftp://ftp.neb.com/pub/rebase/
MIRROR_INCLUDE = (bairoch|commdata)\.[0-9]*$$|VERSION

ZIPFILES = $(wildcard bairoch.* commdata.*)

include make.post

