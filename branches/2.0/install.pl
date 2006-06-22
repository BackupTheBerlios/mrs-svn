#!/usr/bin/perl
# 
# installation script for an MRS (web)site.
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use warnings;
use English;
use Sys::Hostname;
use Config;

$| = 1;	# flush stdout

# some globals

my $perlpath;		# path to the Perl interpreter
my $binpath;		# installation directory for the other executables (mrs.pl and mrs_blast)
my $httpd;			# path to apache webserver
my $httpd_conf;		# path to apache webserver config file
my $site_dir;		# where to install the website
my $data_dir;		# where to store the data files
my $os;				# the OS we're installing on
my $cpu;			# the CPU we're using here

# let's start with a sanity check

die "This version of perl is not supported, please make sure you use at least 5.8\n"
	unless $PERL_VERSION ge v5.8.0;

print "Not running as root, assuming you have sudo installed and configured.\n"
	unless $EFFECTIVE_USER_ID == 0;

# Now first find out what Perl executable will be used

$perlpath = $Config{perlpath};
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
	'CGI'						=> 1,
	'ExtUtils::MakeMaker'		=> 1,
	'Net::FTP'					=> 1,
	'Getopt::Std'				=> 1,
	'Time::HiRes'				=> 1,
	'Time::Local'				=> 1,
	'File::stat'				=> 1,
	'XML::SAX'					=> 1,
	'XML::SAX::Expat'			=> 1,
	'Data::UUID'				=> 1,
	'XML::XSLT'					=> 1,
	'URI'						=> 1,
	'XMLRPC::Transport::HTTP'	=> 0,
	'SOAP::Lite'				=> 0,
	'SOAP::Transport::HTTP'		=> 0,
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

$binpath = &ask_for_string('Where to install other executables', '/usr/local/bin');

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
	$gcc_version = &compile_and_catch($C_file, $cc);
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
	my $tr1_ok = &compile_and_catch($C_file, $cc);
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

my ($boost_location, $boost_version);

eval {
	$boost_version = &compile_and_catch($C_file, $cc);
};

if ($@ or not defined $boost_version or length($boost_version) == 0) {
	foreach my $d ( '/usr/include/boost', '/usr/local/include/boost', '/opt/local/include/boost',
		'/usr/pkg/include/boost' )
	{
		if (-d $d) {
			my $bd = $d;
			$bd =~ s|/[^/]+$||;
			
			eval { $boost_version = &compile_and_catch($C_file, "$cc -I$bd"); };
			next if ($@);
			$boost_location = $bd;
			last;
		}
	}
#	
#	$boost_location = &ask_for_string("Where is boost installed", $boost_location)
#		unless defined $boost_version;
}

die "Cannot continue since you don't seem to have boost installed\nBoost can be found at http://www.boost.org/\n"
	unless defined $boost_version;

# Now write a make.config file in the plugin directory

open MCFG, ">plugin/make.config" or die "Could not open the file make.config for writing!\n";

print MCFG "CC = $cc\n";
print MCFG "SWIG = $swig\n" if defined $swig;
print MCFG "BOOST = $boost_location\n" if defined $boost_location;
print MCFG "HAVE_TR1 = $use_tr1\n";
print MCFG "IQUOTE = 0\n" if $gcc_version < 4;	# use -I- in case of older compiler
print MCFG "INSTALL_DIR = $binpath\n";
print MCFG "SUDO = sudo\n" if $EFFECTIVE_USER_ID != 0;
print MCFG "TARGET = perl5\n";
print MCFG "PERL = $perlpath\n";

close MCFG;

# OK, so compiling and installing the plugins and executables should now work fine.

print "\nCreating the make configuration is now finished.\n";
print "You can cd to the plugin directory and type 'make' followed by 'make install'\n";
print "to create and install the binaries\n\n";

# continue hacking the web scripts

sub ask_for_string {
	my ($question, $default) = @_;
	
	print "$question [$default]: ";
	my $answer = <>;
	chomp($answer);
	
	$answer = $default unless defined($answer) and length($answer) > 0;
	
	return $answer;
}

sub compile_and_catch {
	my ($code, $cc) = @_;
	
	my $r;
	
	my $bn = "gcc_test_$$";
	
	open F, ">/tmp/$bn.cpp";
	print F $C_file;
	close F;

	`$cc -o /tmp/$bn.out /tmp/$bn.cpp 2>&1`;
	
	die "Could not compile" unless -x "/tmp/$bn.out";
	
	$r = `/tmp/$bn.out`;
	chomp($r) if defined $r;

	unlink("/tmp/$bn.out", "/tmp/$bn.cpp");
	
	return $r;
}
