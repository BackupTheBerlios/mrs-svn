#!perl
#
# Use this script to generate an include file that contains rules
# to build multi part MRS files.

use strict;
use warnings;

my $templ = q{
${MRSLIBS:%%=$(MRSDIR)%%-%d.cmp}: $(ZIPFILES)
	$(MRS_CREATE) -d ${@F:%%-%d.cmp=%%} -p %d -P $(MRS_PARTS) $(MRS_OPTIONS)
};

for (my $i = 1; $i <= 32; ++$i) {
	printf($templ, $i, $i, $i);
	
}

$templ = q{
ifeq ($(MRS_PARTS),%d)
PARTS	= %s
endif
};

for (my $i = 1; $i <= 32; ++$i) {
	printf($templ, $i, join(' ', ( 1 .. $i )));
}
