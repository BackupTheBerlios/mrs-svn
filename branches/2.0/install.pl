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

die "\nThis version of perl is not supported, please make sure you use at least 5.8\n"
	unless $PERL_VERSION ge v5.8.0;

print "\nNot running as root, this will probably fail but we'll see how far we will get.\n\n"
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
	'Sys::Proctitle'			=> 0,
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
	print "\nMissing modules: ", join(", ", @missing_modules), "\n";
	
	die "Please install these before continuing installation\n" if ($essential);
	warn "The missing modules are not essential, but functionality may be limited\n\n";
}

# Then ask the user for the installation directory for the other tools

$binpath = &ask_for_string('Where to install other executables', '/usr/local/bin');

# Try to find out some variables needed for building the plugin
# First see if the right version of SWIG is available
my $swig = `which swig`;
chomp($swig);

$swig = &ask_for_string("\nWhere is your version of swig (can be left empty)", $swig);

if (defined $swig and length($swig) > 0) {

	my $swig_version;
	open P, "$swig -version|";
	while (my $vl = <P>) {
		if ($vl =~ m|SWIG Version (.+)|) {
			$swig_version = $1;
			last;
		}
	}
	close P;
	
	if ($swig_version ne '1.3.27') {
		warn "Swig version 1.3.27 (exactly) is needed to build MRS\n";
		undef $swig;
	}
}

# Next see if we have a decent version of gcc

my $gcc_version;
my $cc = `which c++`;
chomp($cc);
$cc = `which gcc` unless defined $cc and length($cc) > 0;
chomp($cc);

$cc = &ask_for_string("\nWhich recent c++ compiler do you want to use", $cc);

my $C_file=<<END;
#include <iostream>
int main() { std::cout << __GNUC__ << std::endl; }
END

chdir("/tmp");
open F, ">gcc_test.cpp";
print F $C_file;
close F;
eval {
	`$cc -o gcc_test.out gcc_test.cpp`;
	$gcc_version = `./gcc_test.out`;
	chomp($gcc_version);
	unlink('gcc_test.out', 'gcc_test.cpp');
};

if ($@) {
	warn "Something went wrong detecting the c++ compiler: $@\nWill try to compile anyway\n";
	$cc = 'cc';
	$gcc_version = undef;
}

if (defined $gcc_version) {
	if ($gcc_version >= 4) {
		print "\nExcellent! Found a decent version of GCC\n";
	}
	elsif ($gcc_version >= 3) {
		print "\nFound an old (pre 4.x) version of GCC, will try to use it but this may fail\n";
	}
	else {
		die "This version of gcc is way too old to build MRS\n";
	}
}

sub ask_for_string {
	my ($question, $default) = @_;
	
	print "\n$question [$default]: ";
	my $answer = <>;
	chomp($answer);
	
	$answer = $default unless defined($answer) and length($answer) > 0;
	
	return $answer;
}

