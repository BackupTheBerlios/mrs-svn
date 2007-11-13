#!/usr/bin/perl
# 
# configuration script for MRS
#
# Copyright 2007, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#
#	This script works a bit like the GNU configure scripts, it checks whether
#	the required libraries are available and some other info.
#	
#

use strict;
use warnings;
use English;
use Config;
use Sys::Hostname;
use Getopt::Long;
use Data::Dumper;

$| = 1;	# flush stdout

# let's start with a sanity check

die "This version of perl is not supported, please make sure you use at least 5.8\n"
	unless $PERL_VERSION ge v5.8.0;

# Now first find out what Perl executable will be used

my $perlpath = $Config{perlpath};
if ($OSNAME ne 'VMS') {
	$perlpath .= $Config{_exe}
		unless $perlpath =~ m/$Config{_exe}$/i;
}

print "Using perl $perlpath\n";

my %lib_dirs;	# the set of library search paths

my @lib_dirs_guess = (
	'/usr/lib',
	'/usr/lib64',
	'/usr/local/lib',
	'/usr/local/lib64',
	'/usr/pkg/lib',
	'/usr/pkg/lib64',
	'/opt/local/lib',
	'/opt/local/lib64'
);

my %inc_dirs;	# the set of include search paths

my @inc_dirs_guess = (
	'/usr/include',
	'/usr/local/include',
	'/usr/pkg/include',
	'/opt/local/include',
);


my $use_tr1 = 0;

my $os = `uname -s`;	chomp($os);
my $host = hostname;
my $cpu = `uname -p`;	chomp($cpu);
$cpu = `uname -m` if $cpu eq 'unknown';

my $maintainer = `whoami`;
chomp($maintainer);
$maintainer = "$maintainer\@$host";

# variables that can be set using flags to ./configure:

# The prefix directory
my $prefix = "/usr/local";

# The data directory
my $data_dir = "prefix/data";

# The MRS parser scripts directory
my $parser_scripts = "prefix/share/mrs/parser_scripts";

# The MRS update scripts directory
my $update_scripts = "prefix/share/mrs/update_scripts";

# the compiler to use
my $cc = $ENV{CC};
$cc = $ENV{CXX} unless defined $cc;
$cc = `which c++` unless defined $cc;
chomp($cc);

my ($gcc_version, $gcc_march);

# The SWIG executable
my $swig = `which swig`;
chomp($swig);

# The soapcpp2 executable
my $soapcpp2 = `which soapcpp2`;
chomp($soapcpp2);

# The clustalw executable
my $clustalw = `which clustalw`;
chomp($clustalw);

# The boost variables
my $boost_dir;
my $boost_regex = "boost_regex-mt";
my $boost_filesystem = "boost_filesystem-mt";
my $boost_thread = "boost_thread-mt";

my $no_blast = 0;

# That's it, now check the options
&GetOptions(
	"prefix=s"				=> \$prefix,
	"data-dir=s"			=> \$data_dir,
	"parser-script-dir=s"	=> \$parser_scripts,
	"update-script-dir=s"	=> \$update_scripts,
	"cc=s"					=> \$cc,
	"swig=s"				=> \$swig,
	"soapcpp2=s"			=> \$soapcpp2,
	"boost=s"				=> \$boost_dir,
	"boost_regex=s"			=> \$boost_regex,
	"boost_filesystem=s"	=> \$boost_filesystem,
	"boost_thread=s"		=> \$boost_thread,
	"no-blast"              => \$no_blast,
	"gcc-march-flag=s"      => \$gcc_march,
	"help"					=> \&Usage);

$data_dir =~ s|^prefix/|$prefix/|;
$parser_scripts =~ s|^prefix/|$prefix/|;
$update_scripts =~ s|^prefix/|$prefix/|;

# a usage subroutine

sub Usage {
print<<EOF;
Usage: configure
	--help				This help message
	--prefix			The installation directory prefix string
	--data-dir			The data directory, where all data files will be located.
						This should probably be a separate partition.
						Default is $data_dir.
	--parser-script-dir	The directory containing the MRS parser scripts
						Default is $prefix/share/mrs/parser_scripts
	--update-script-dir	The directory containing the MRS update scripts
						Default is $prefix/share/mrs/update_scripts
	--cc				The compiler to use, perferrably a GCC >= 4.1
						Default is $cc
	--gcc-march-flag=[] The -march=xx flag for gcc. This is used to build
	                    an optimized version of MRS code specifically for
	                    an architecture. Examples are 'nocona', 'prescott'
	                    and 'pentium4'.
	--swig				The location of the SWIG executable. This executable
						must be version 1.3.27 exactly. Other versions are not
						supported.
						Default is $swig
	--soapcpp2			The location of the soapcpp2 executable which is part of
						the gSoap toolset.
						Default is $soapcpp2
	--boost=[path]		The path where the boost libraries are installed
	--boost_regex=[]	The name of the boost_regex library
						Default is boost_regex-mt
	--boost_filesystem=[] The name of the boost_filesystem library
						Default is boost_filesystem-mt
	--boost_thread=[]	The name of the boost_thread library
						Default is boost_thread-mt
	--no-blast          Do not compile the BLAST code.
	
EOF
	exit;
}

#/

# OK, we have the options, now validate them and then try to build the makefiles and config files

$| = 1;	# flush stdout

&CheckPerlModules();
&CheckMachine();
&ValidateOptions();
&WriteMakefiles();
exit;

# Check
sub CheckPerlModules()
{
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
		print "Checking for Perl module $m...";
		eval "require $m;";
		print " OK\n";
		
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
}

sub CheckMachine()
{
	# find out the OS and CPU we're using
	
	if ($cpu eq 'x86_64' or $cpu =~ m/i[456]86/) {
		$cpu = 'x86';
	}
	elsif ($cpu eq 'ppc' or $cpu eq 'powerpc') {
		$cpu = 'ppc';
	}
	else {
		die "Unsupported CPU: $cpu\nPlease report this problem to m.hekkelman\@cmbi.ru.nl\n";
	}
	
	
}

sub ValidateOptions()
{
	# Check the compiler
	
	print "Checking for a compiler...";

	my $C_file=<<END;
#include <iostream>
int main() { std::cout << __GNUC__ << std::endl; return 0; }
END
	
	eval {
		$gcc_version = &compile($C_file, $cc);
	};
	
	if ($@) {
		die "A compiler error prevented the determination of the version: $@\n";
	}
	
	if (defined $gcc_version) {
		if ($gcc_version >= 4) {
			print " OK\n";
		}
		elsif ($gcc_version >= 3) {
			print " Hmmmm\nFound an old (pre 4.x) version of GCC, will try to use it but this will probably fail\n";
		}
		else {
			die "\nThis version of gcc is too old to build MRS\n";
		}
	}
	
	# Check the SWIG version
	
	print "Checking the version of SWIG...";
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
			print "\nVersion $swig_version of Swig is not the correct one\n"
				if defined $swig_version;
			print "Swig version 1.3.27 is needed to build MRS\n";
			undef $swig;
		}
	}
	
	if (defined $swig) {
		print " OK\n";
	}
	else {
		print " Skipping\n";
	}
	
	# Check the existence of soapcpp2

	print "Checking the version of soapcpp2\n";
	if (defined $soapcpp2 and length($soapcpp2) > 0) {
		my $soapcpp2_version;
		open P, "$soapcpp2 -v 2>&1 |";
		while (my $vl = <P>) {
			if ($vl =~ m|\*\*\s+The gSOAP Stub and Skeleton Compiler for C and C\+\+\s+(.+)|) {
				$soapcpp2_version = $1;
				last;
			}
		}
		close P;
		
		if (not defined $soapcpp2_version or substr($soapcpp2_version, 0, 4) ne '2.7.') {
			$soapcpp2_version = "'unknown'" unless defined $soapcpp2_version;
			print "Version $soapcpp2_version of gSoap is not tested\ngSoap version 2.7 was used for development of mrsws\n"
				if substr($soapcpp2_version, 0, 4) ne '2.7.';
		}
	}
	
	# check if we can compile a soap server
	
	$C_file=<<END;
#include <stdsoap2.h>
int main() { struct soap S; return 0; }
END
	# first locate the libraries we need: libgsoap.a or libgsoap.so
	my $link_ok = 0;
	
	foreach my $lib_dir (@lib_dirs_guess) {
		next unless -e "$lib_dir/libgsoap.a" or -e "$lib_dir/libgsoap.so";
		
		my $ld_opts = "-L$lib_dir -lgsoap";
		
		eval {
			compile($C_file, "$cc -x c", $ld_opts);
		};
		
		next if $@;
		
		$lib_dirs{$lib_dir} = 1;
		$link_ok = 1;
		
		last;
	}

	if (not $link_ok) {
		die "\nCould not build a soap application\n";
	}
	
	# TR1

	print "Checking for the availability of tr1...";
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
		print "\nYou do not seem to have the TR1 headers installed\n";
	}
	else {
		print " OK\n";
	}
	
	# boost libraries
	
	print "Checking the boost header files...";
	
	$C_file =<<END;
#include <boost/version.hpp>
#include <iostream>
int main() { std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END
	
	my ($boost_version, $boost_inc);
	
	eval {
		my $c_opts = $cc;
		$c_opts = "$cc -I$boost_dir" if defined $boost_dir;
		
		$boost_version = &compile($C_file, $c_opts);
	};
	
	if ($@ or not defined $boost_version or length($boost_version) == 0) {
		foreach my $d ( @inc_dirs_guess )
		{
			if (-d "$d/boost") {
				eval { $boost_version = &compile($C_file, "$cc -I$d"); };
				next if ($@);
				$inc_dirs{$d} = 1;
				last;
			}
		}
	}
	
	if (not defined $boost_version or length($boost_version) == 0) {
		die "\nBoost could not be found, please specify using --boost=/... option\n";
	}
	else {
		print " OK\n";
	}

	# OK, so boost is installed, but is boost_regex and booost_filesystem installed as well?
	
	print "Checking for boost_regex library...";
	$C_file =<<END;
#include <boost/version.hpp>
#include <boost/regex.hpp>
#include <iostream>
int main() { boost::regex re("."); std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END
	
	my $boost_lib_ok = 0;
	
	foreach my $lib_dir ( @lib_dirs_guess )
	{
		next unless -e "$lib_dir/lib${boost_regex}.a" or -e "$lib_dir/lib${boost_regex}.so";

		eval {
			my $test = &compile($C_file, "$cc", " -l${boost_regex}");
			die "Boost versions do not match\n" unless $test eq $boost_version;
		};
	
		next if ($@);
		
		$lib_dirs{$lib_dir} = 1;
		$boost_lib_ok = 1;
		last;
	}
	
	if (not $boost_lib_ok) {
		die "\nthe boost library boost_regex seems to be missing\n",
			"Please specify using the --boost_regex option\n";
	}
	else {
		print " OK\n";
	}

	print "Checking for boost_filesystem library...";
	$C_file =<<END;
#include <boost/version.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
int main() { boost::filesystem::path p("/tmp"); std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END
	
	$boost_lib_ok = 0;
	
	foreach my $lib_dir ( @lib_dirs_guess )
	{
		next unless -e "$lib_dir/lib${boost_filesystem}.a" or -e "$lib_dir/lib${boost_filesystem}.so";

		eval {
			my $test = &compile($C_file, "$cc", " -l${boost_filesystem}");
			die "Boost versions do not match\n" unless $test eq $boost_version;
		};
	
		next if ($@);
		
		$lib_dirs{$lib_dir} = 1;
		$boost_lib_ok = 1;
		last;
	}
	
	if (not $boost_lib_ok) {
		die "\nthe boost library boost_filesystem seems to be missing\n",
			"Please specify using the --boost_filesystem option\n";
	}
	else {
		print " OK\n";
	}

	print "Checking for boost_thread library...";
	$C_file =<<END;
#include <boost/version.hpp>
#include <boost/thread.hpp>
#include <iostream>
int main() { boost::thread_group thrg; std::cout << BOOST_LIB_VERSION << std::endl; return 0; }
END
	
	$boost_lib_ok = 0;
	
	foreach my $lib_dir ( @lib_dirs_guess )
	{
		next unless -e "$lib_dir/lib${boost_thread}.a" or -e "$lib_dir/lib${boost_thread}.so";

		eval {
			my $test = &compile($C_file, "$cc", " -l${boost_thread}");
			die "Boost versions do not match\n" unless $test eq $boost_version;
		};
	
		next if ($@);
		
		$lib_dirs{$lib_dir} = 1;
		$boost_lib_ok = 1;
		last;
	}
	
	if (not $boost_lib_ok) {
		die "\nthe boost library boost_thread seems to be missing\n",
			"Please specify using the --boost_thread option\n";
	}
	else {
		print " OK\n";
	}

	# libxml2

	print "Checking for libxml2...";
	$C_file =<<END;
#include <libxml/xmlversion.h>
#include <iostream>
int main() { std::cout << LIBXML_DOTTED_VERSION << std::endl; return 0; }
END

	my ($libxml_lib, $libxml_version);
	
	eval {
		$libxml_version = &compile($C_file, $cc);
	};
	
	if ($@ or not defined $libxml_version) {
		foreach my $d ( @inc_dirs_guess )
		{
			if (-d "$d/libxml2") {
				eval { $libxml_version = &compile($C_file, "$cc -I$d/libxml2"); };
				next if ($@);
				$inc_dirs{"$d/libxml2"} = 1;
				last;
			}
		}
	}
	
	if (defined $libxml_version) {
	
		$C_file =<<END;
#include <libxml/xmlversion.h>
#include <iostream>
int main() { LIBXML_TEST_VERSION; return 0; }
END

		foreach my $lib_dir ( @lib_dirs_guess )
		{
			$libxml_version = undef;

			eval {
				$libxml_version = &compile($C_file, "$cc", " -lxml2");
			};
		
			next if ($@);

			$lib_dirs{$lib_dir} = 1;
			last;
		}
	}
	
	if (not defined $libxml_version) {
		die "Cannot continue since you don't seem to have libxml2 installed\n",
			"libxml2 can be found at http://xmlsoft.org/\n"
	}
	else {
		print " OK\n";
	}
}

sub WriteMakefiles()
{
	# Now write a make.config file in the plugin directory
	
	print "Creating make.config file...";
	
	$swig = "" unless defined $swig;
	
	my $inc_paths = join(" ", keys %inc_dirs);
	my $lib_paths = join(" ", keys %lib_dirs);
	
	my $libs = "xml2 $boost_regex $boost_filesystem $boost_thread";
	
	my $etc_dir = "$prefix/etc";
	my $bin_dir = "$prefix/bin";
	my $lib_dir = "$prefix/lib";
	
	my $iquote = 1;
	if (defined $gcc_version and $gcc_version < 4) {
		$iquote = 0;
	}
	
	my $no_blast_def = "";
	$no_blast_def = "DEFINES       += NO_BLAST\n" if $no_blast;

	my $opt = "	# e.g. -march=nocona for Core 2 processors, but be careful";
	$opt = "-march=$gcc_march" if defined $gcc_march;

	open MCFG, ">make.config" or die "Could not open the file make.config for writing!\n";
	
print MCFG<<EOF;
# These variables were set up by the configure.pl script
# 

# general info

CPU           = $cpu
OS            = $os
HOST          = $host
MAINTAINER    = $maintainer

# Applications (compilers, interpreters, etc) to use

CC            = $cc
SWIG          = $swig
SOAPCPP2      = $soapcpp2
PERL          = $perlpath
CLUSTALW      = $clustalw

# include paths and libraries

SYSINCPATHS   += $inc_paths
LIBPATHS      += $lib_paths
LIBS          += $libs

# compiler flags and defines

IQUOTE        = $iquote
DEFINES       += MRS_CONFIG_FILE='"$etc_dir/mrs-config.xml"'
DEFINES       += MRS_DATA_DIR='"$data_dir/mrs"'
DEFINES       += MRS_PARSER_DIR='"$parser_scripts"'
DEFINES       += HAVE_TR1=$use_tr1
OPT           += $opt
$no_blast_def
# paths

PREFIX        = $prefix
BINDIR        = $bin_dir
LIBDIR        = $lib_dir
DATADIR       = $data_dir
P_SCRIPTDIR   = $parser_scripts
U_SCRIPTDIR   = $update_scripts
EOF
	
	close MCFG;
	
	print " done\n";
}

# utility routines
sub compile {
	my ($code, $cc, $ld_options) = @_;
	
	foreach my $inc_dir (keys %inc_dirs) {
		$cc = "$cc -I$inc_dir";
	}
	
	$ld_options = "" unless defined $ld_options;
	
	foreach my $lib_dir (keys %lib_dirs) {
		$ld_options = "$ld_options -L$lib_dir";
	}
	
	my $r;
	our ($n);

	++$n;
	
	my $bn = "gcc_test_$$-$n";
	
	open F, ">/tmp/$bn.cpp";
	print F $code;
	close F;

	my $err = `$cc -o /tmp/$bn.out /tmp/$bn.cpp $ld_options 2>&1`;
	
	die "Could not compile: $err\n" unless -x "/tmp/$bn.out";
	
	$r = `/tmp/$bn.out`;
	chomp($r) if defined $r;

	unlink("/tmp/$bn.out", "/tmp/$bn.cpp");
	
	return $r;
}

