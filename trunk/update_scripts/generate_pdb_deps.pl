#!/bin/perl

my $db = shift or die "No db specified";
my $srcdir = shift;
my $dstdir = shift;

die "no valid source directory" unless -d $srcdir;
die "no valid destination directory" unless -d $dstdir;

open DEP, ">$db.deps";
open LST, "find $srcdir -name \"*.Z\" |";

while (<LST>) {
	if (/(.+)\/([a-z0-9]{2})\/((pdb|r)?\2?.+)\.Z/) {
		print DEP "$dstdir$3: $1/$2/$3.Z\n";
		print DEP "\tzcat \$< > \$@\n";
		print DEP "\ttouch -cr \$< \$@\n";
		print DEP "\n";
	}
}

close LST;
close DEP;
