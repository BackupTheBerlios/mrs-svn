#!/usr/bin/perl -w

use strict;

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
