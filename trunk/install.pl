#!/usr/bin/perl
# 
# installation script for an MRS (web)site.
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use warnings;
use English;
use Config;

$| = 1;	# flush stdout

# let's start with a sanity check

die "This version of perl is not supported, please make sure you use at least 5.8\n"
	unless $PERL_VERSION ge v5.8.0;

warn "Not running as root, assuming you have sudo installed and configured.\n"
	unless $EFFECTIVE_USER_ID == 0;

# Now first find out what Perl executable will be used

my $perlpath = $Config{perlpath};
if ($OSNAME ne 'VMS') {
	$perlpath .= $Config{_exe}
		unless $perlpath =~ m/$Config{_exe}$/i;
}

while (1) {
	my $a = &ask_for_string("Using $perlpath as your Perl interpreter. Is this correct?", "yes");
	
	if ($a eq 'no' or $a eq 'n') {
		die "Please restart the script with the correct version of Perl\n";
	}
	
	if ($a ne "yes" and $a ne "no") {
		$a = &ask_for_string("Please enter yes or no", "yes");
	}

	last;
}	

# Find out if all the required modules are there:

my %modules = (
	'CGI'						=> 1,		# for mrsws, the soap server
	'ExtUtils::MakeMaker'		=> 1,		# for building the plugin
	'Net::FTP'					=> 1,		# for the mirror.pl script
	'Getopt::Std'				=> 1,		# idem
	'URI'						=> 1,		# idem
);

my @missing_modules;
my $essential = 0;

foreach my $m (keys %modules) {
	eval "require $m;";
	
	if ($@) {
		push @missing_modules, $m;
		$essential += $modules{$m};
	}
}

if (scalar @missing_modules > 0) {
	print "Missing modules: ", join(", ", @missing_modules), "\n";
	
	die "Please install these before continuing installation\n" if ($essential);
	print "The missing modules are not essential, but functionality may be limited\n\n";
}

# Then ask the user for the installation directory for the other tools

my $binpath = &ask_for_string('Where to install other executables', '/usr/local/bin');

# Try to find out some variables needed for building the plugin
# First see if the right version of SWIG is available
my $swig = `which swig`;
chomp($swig);

$swig = &ask_for_string("Where is your version of swig (can be left empty)", $swig);

if (defined $swig and length($swig) > 0) {
	my $swig_version;
	open P, "$swig -version 2>&1 |";
	while (my $vl = <P>) {
		if ($vl =~ m|SWIG Version (.+)|) {
			$swig_version = $1;
			last;
		}
	}
	close P;
	
	if (not defined $swig_version or $swig_version ne '1.3.27') {
		print "Version $swig_version of Swig is not the correct one\n"
			if defined $swig_version;
		print "Swig version 1.3.27 is needed to build MRS\n";
		undef $swig;
	}
}

# For building the web service application we really need gSoap
my $gsoap = `which soapcpp2`;
chomp($gsoap);

$gsoap = &ask_for_string("Where is your version of gSoap's soapcpp2", $gsoap);

if (defined $gsoap and length($gsoap) > 0) {
	my $gsoap_version;
	open P, "$gsoap -v 2>&1 |";
	while (my $vl = <P>) {
		if ($vl =~ m|\*\*\s+The gSOAP Stub and Skeleton Compiler for C and C\+\+\s+(.+)|) {
			$gsoap_version = $1;
			last;
		}
	}
	close P;
	
	if (not defined $gsoap_version or $gsoap_version ne '2.7.8c') {
		$gsoap_version = "'unknown'" unless defined $gsoap_version;
		print "Version $gsoap_version of gSoap is not tested\ngSoap version 2.7.8c was used for development of mrsws\n"
			if $gsoap_version ne '2.7.8c';
	}
}

# Next see if we have a decent version of gcc

my $gcc_version;
my $cc = `which c++`;
chomp($cc);
$cc = `which gcc` unless defined $cc and length($cc) > 0;
chomp($cc);

$cc = &ask_for_string("Which c++ compiler to use (gcc 4.1 is preferred)", $cc);

my $C_file=<<END;
#include <iostream>
int main() { std::cout << __GNUC__ << std::endl; return 0; }
END

eval {
	$gcc_version = &compile($C_file, $cc);
};

if ($@) {
	$cc = 'cc';
	$gcc_version = undef;
	print "A compiler error prevented the determination of the version: $@\n";
}

if (defined $gcc_version) {
	if ($gcc_version >= 4) {
#		print "Excellent! Found a decent version of GCC\n";
	}
	elsif ($gcc_version >= 3) {
		print "Found an old (pre 4.x) version of GCC, will try to use it but this may fail\n";
	}
	else {
		die "This version of gcc is way too old to build MRS\n";
	}
}

# See if the compiler has TR1 headers installed
my $use_tr1 = 0;

$C_file=<<END;
#include <tr1/unordered_set>
#include <iostream>
int main() { std::cout << "ok" << std::endl; return 0; }
END

eval {
	my $tr1_ok = &compile($C_file, $cc);
	$use_tr1 = 1 if $tr1_ok eq 'ok';
};

if ($@) {
	print "You do not seem to have the TR1 headers installed\n";
}

# Tricky one, see if there's a boost installed

$C_file =<<END;
#include <boost/version.hpp>
#include <iostream>
int main() { std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END

my ($boost_inc, $boost_lib, $boost_lib_suffix, $boost_version);

eval {
	$boost_version = &compile($C_file, $cc);
};

if ($@ or not defined $boost_version or length($boost_version) == 0) {
	foreach my $d ( '/usr/include', '/usr/local/include', '/usr/local/include/boost-1_33_1/', '/opt/local/include',
		'/usr/pkg/include' )
	{
		if (-d "$d/boost") {
			eval { $boost_version = &compile($C_file, "$cc -I$d"); };
			next if ($@);
			$boost_inc = $d;
			last;
		}
	}
#	
#	$boost_inc = &ask_for_string("Where is boost installed", $boost_inc)
#		unless defined $boost_version;
}

die "Cannot continue since you don't seem to have boost installed\nBoost can be found at http://www.boost.org/\n"
	unless defined $boost_version;

# OK, so boost is installed, but is boost_regex and booost_filesystem installed as well?

$C_file =<<END;
#include <boost/version.hpp>
#include <boost/regex.hpp>
#include <iostream>
int main() { boost::regex re("."); std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END

my $boost_lib_ok = 0;
$boost_lib_suffix = "";

foreach my $d ( undef, '/usr/lib', '/usr/local/lib', '/opt/local/lib',
	'/usr/pkg/lib', '/usr/lib64' )
{
	next unless -e "$d/libboost_regex.so" or -e "$d/libboost_regex-gcc.so";
	$boost_lib_suffix = "-gcc" if -e "$d/libboost_regex-gcc.so" and not -e "$d/libboost_regex.so";

	eval {
		my $b_cc = $cc;
		$b_cc .= " -I$boost_inc " if defined $boost_inc;
		$b_cc .= " -L$d " if defined $d;
		my $test = &compile($C_file, "$b_cc -lboost_regex$boost_lib_suffix");
		die "Boost versions do not match\n" unless $test eq $boost_version;
	};

	next if ($@);
	$boost_lib = $d;
	$boost_lib_ok = 1;
	last;
}

die "the boost library boost_regex seems to be missing" unless $boost_lib_ok;

$C_file =<<END;
#include <boost/version.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
int main() { boost::filesystem::path p; return 0; }
END

eval {
	my $b_cc = $cc;
	$b_cc .= " -I$boost_inc " if defined $boost_inc;
	$b_cc .= " -L$boost_lib " if defined $boost_lib;
	&compile($C_file, "$b_cc -lboost_filesystem$boost_lib_suffix");
};

if ($@ or not defined $boost_version or length($boost_version) == 0) {
	die "the boost library boost_filesystem seems to be missing";
}

# we also need libxml2, see if that is installed as well

$C_file =<<END;
#include <libxml/xmlversion.h>
#include <iostream>
int main() { std::cout << LIBXML_DOTTED_VERSION << std::endl; return 0; }
END

my ($libxml_inc, $libxml_lib, $libxml_version);

eval {
	$libxml_version = &compile($C_file, $cc);
};

if ($@ or not defined $libxml_version) {
	foreach my $d ( '/usr/include', '/usr/local/include', '/opt/local/include',
		'/usr/pkg/include' )
	{
		if (-d "$d/libxml2") {
			eval { $libxml_version = &compile($C_file, "$cc -I$d/libxml2"); };
			next if ($@);
			$libxml_inc = "$d/libxml2";
			last;
		}
	}
}

die "Cannot continue since you don't seem to have libxml2 installed\nlibxml2 can be found at http://xmlsoft.org/\n"
	unless defined $libxml_version;

$C_file =<<END;
#include <libxml/xmlversion.h>
#include <iostream>
int main() { LIBXML_TEST_VERSION; return 0; }
END

my $xml_lib_ok = 0;

foreach my $d ( undef, '/usr/lib', '/usr/local/lib', '/opt/local/lib',
	'/usr/pkg/lib', '/usr/lib64' )
{
	eval {
		my $x_cc = $cc;
		$x_cc .= " -I$libxml_inc " if defined $libxml_inc;
		$x_cc .= " -L$d " if defined $d;
		&compile($C_file, "$x_cc -lxml2");
	};

	next if ($@);
	$libxml_lib = $d;
	$xml_lib_ok = 1;
	last;
}

die "the boost library boost_regex seems to be missing" unless $xml_lib_ok;

# Now write a make.config file in the plugin directory

open MCFG, ">plugin/make.config" or die "Could not open the file plugin/make.config for writing!\n";

print MCFG "CC = $cc\n";
print MCFG "SWIG = $swig\n" if defined $swig;
print MCFG "SYSINCPATHS += $boost_inc\n" if defined $boost_inc;
print MCFG "LIBPATHS += $boost_lib\n" if defined $boost_lib;
print MCFG "BOOST_LIB_SUFFIX = $boost_lib_suffix\n" if length($boost_lib_suffix) > 0;
print MCFG "HAVE_TR1 = $use_tr1\n";
print MCFG "IQUOTE = 0\n" if $gcc_version < 4;	# use -I- in case of older compiler
print MCFG "INSTALL_DIR = $binpath\n";
print MCFG "SUDO = sudo\n" if $EFFECTIVE_USER_ID != 0;
print MCFG "TARGET = perl5\n";
print MCFG "PERL = $perlpath\n";

close MCFG;

# and a make.config file in the ws directory

open MCFG, ">ws/make.config" or die "Could not open the file ws/make.config for writing!\n";

print MCFG "CC = $cc\n";
print MCFG "SYSINCPATHS += $boost_inc\n" if defined $boost_inc;
print MCFG "LIBPATHS += $boost_lib\n" if defined $boost_lib;
print MCFG "BOOST_LIB_SUFFIX = $boost_lib_suffix\n" if length($boost_lib_suffix) > 0;
print MCFG "SYSINCPATHS += $libxml_inc\n" if defined $libxml_inc;
print MCFG "LIBPATHS += $libxml_lib\n" if defined $libxml_lib;
print MCFG "HAVE_TR1 = $use_tr1\n";
print MCFG "IQUOTE = 0\n" if $gcc_version < 4;	# use -I- in case of older compiler
print MCFG "INSTALL_DIR = $binpath\n";
print MCFG "SUDO = sudo\n" if $EFFECTIVE_USER_ID != 0;
print MCFG "SOAPCPP2 = $gsoap\n" if defined $gsoap;
print MCFG "PERL = $perlpath\n";

close MCFG;

# OK, so compiling and installing the plugins and executables should now work fine.

print "\nCreating the make configuration is now finished.\n";
print "You can type 'make' followed by 'make install' to create and install the binaries\n\n";

# continue hacking the update scripts

print "\n";
print "The update scripts will be installed next. Please specify the directory\n";
print "where the data should be stored. If you want to have EMBL and Genbank you\n";
print "will need something like 750 Gb of free diskspace in this directory.\n\n";

my $data_dir = &ask_for_string('Where should the data be located', '/usr/local/data/');
&mkdir_recursive($data_dir);

my $make_script_dir = &ask_for_string("Where should the update scripts go", '/usr/local/share/mrs/update_scripts');
&mkdir_recursive($make_script_dir);

my $parser_script_dir = &ask_for_string("Where should the parser scripts go", '/usr/local/share/mrs/parser_scripts');
&mkdir_recursive($parser_script_dir);

my $maintainer = `whoami`;
chomp($maintainer);
&ask_for_string("What is the e-mail address for the maintainer", $maintainer);

my $db_conf = &read_file("update_scripts/make_TEMPLATE.conf");

# clean up all our path strings, they should all end in exactly one slash
$data_dir			=~ s|/*$|/|;
$binpath			=~ s|/*$|/|;
$make_script_dir	=~ s|/*$|/|;
$parser_script_dir	=~ s|/*$|/|;

# update the make_xxx.conf file
$db_conf =~ s{__DATA_DIR__}		{$data_dir}g;
$db_conf =~ s{__BIN_DIR__}		{$binpath}g;
$db_conf =~ s{__MAINTAINER__}	{$maintainer}g;
$db_conf =~ s{__SCRIPT_DIR__}	{$make_script_dir}g;
$db_conf =~ s{__PARSER_DIR__}	{$parser_script_dir}g;
$db_conf =~ s{__PERL__}			{$perlpath}g;

my $hostname = `hostname -s`;
chomp($hostname);

my $db_conf_file = "$make_script_dir/make_$hostname.conf";
&write_file($db_conf, $db_conf_file);

# copy over all the files in update_scripts
system("cd update_scripts; find . | cpio -p $make_script_dir") == 0
	or die "Could not copy over the files in update_scripts to $make_script_dir: $!\n";

# copy over all parsers
system("cd parser_scripts; find . | cpio -p $parser_script_dir") == 0
	or die "Could not copy over the files in parser_scripts to $parser_script_dir: $!\n";

print "\nOK, installation seems to have worked fine up until now.\n";
print "Next steps are to build the actual plugins, as stated above you have to\n";
print "enter the following commands for this:\n";
print "\n";
print "cd plugin\n";
print "make\n";
print "make install\n";
print "\n";
print "The make install may fail if you don't have sudo and don't have the permission\n";
print "to install the plugin or executables. In that case you have to run that\n";
print "make install step as root.\n";
print "\n";
print "Once you've installed the plugins you can cd into $make_script_dir and to test the\n";
print "scripts there you could e.g. type:\n";
print "\n";
print "make enzyme\n";
print "\n";
print "That command will create the directories and tools needed to do updates, it then\n";
print "continues fetching the enzyme data files and creates an mrs file from them.\n";
print "\n";
print "Other useful commands you can type in the make directory are:\n";
print "\n";
print "make -j 2 daily		# update all daily dbs using 2 processors\n";
print "make -j 2 weekly		# update all dbs\n";
print "\n";
print "Have a look at the $db_conf_file for local settings.\n";

exit;

sub ask_for_string {
	my ($question, $default) = @_;
	
	print "$question [$default]: ";
	my $answer = <>;
	chomp($answer);
	
	$answer = $default unless defined($answer) and length($answer) > 0;
	
	return $answer;
}

sub read_file {
	my $file = shift;
	local($/) = undef;
	open FILE, "<$file" or die "Could not open file $file for reading";
	my $result = <FILE>;
	close FILE;
	return $result;
}

sub write_file {
	my ($text, $file) = @_;
	open FILE, ">$file" or die "Could not open file $file for writing";
	print FILE $text;
	close FILE;
}

sub compile {
	my ($code, $cc) = @_;
	
	my $r;
	our ($n);

	++$n;
	
	my $bn = "gcc_test_$$-$n";
	
	open F, ">/tmp/$bn.cpp";
	print F $C_file;
	close F;

print "$cc -o /tmp/$bn.out /tmp/$bn.cpp 2>&1\n";
	my $err = `$cc -o /tmp/$bn.out /tmp/$bn.cpp 2>&1`;
	
	die "Could not compile: $err\n" unless -x "/tmp/$bn.out";
	
	$r = `/tmp/$bn.out`;
	chomp($r) if defined $r;

#	unlink("/tmp/$bn.out", "/tmp/$bn.cpp");
	
	return $r;
}

sub mkdir_recursive {
	my $dir = shift;

	$dir =~ s|/+$||;	# strip off trailing slash

	system("mkdir", "-p", $dir);

	return if -d $dir;	# if it exists we're done
		
	eval {
		print "Trying to create directory with sudo and mkdir\n";
		my @args = ("sudo", "mkdir", "-p", "-m755", $dir);
		system(@args) == 0 or die "system @args failed: $?";
		@args = ("sudo", "chown", $REAL_USER_ID, $dir);
		system(@args) == 0 or die "system @args failed: $?";
	};
	
	if ($@) {
		print "Using sudo and mkdir failed, trying to do it the regular way.\n";
		
		my $parent = $dir;
		$parent =~ s|/+$||;
		$parent =~ s|[^/]+$||;
		
		&mkdir_recursive($parent) if (length($parent) > 0);
		
		mkdir($dir) or die "Could not create $dir, please create it and then rerun this script\n";
		chmod 0755, $dir;
	}
}
