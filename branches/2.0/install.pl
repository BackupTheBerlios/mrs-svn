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

# let's start with a sanity check

die "This version of perl is not supported, please make sure you use at least 5.8\n"
	unless $PERL_VERSION ge v5.8.0;

print "Not running as root, assuming you have sudo installed and configured.\n"
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

my $db_conf;
if (1) {
	local($/) = undef;
	open T, "<db-update/make_TEMPLATE.conf" or die "make_TEMPLATE.conf seems to be missing: $@";
	$db_conf = <T>;
	close T;
}

$data_dir			=~ s|/*$|/|;
$binpath			=~ s|/*$|/|;
$make_script_dir	=~ s|/*$|/|;
$parser_script_dir	=~ s|/*$|/|;

$db_conf =~ s|__DATA_DIR__|$data_dir|g;
$db_conf =~ s|__BIN_DIR__|$binpath|g;
$db_conf =~ s|__MAINTAINER__|$maintainer|g;
$db_conf =~ s|__SCRIPT_DIR__|$make_script_dir|g;
$db_conf =~ s|__PARSER_DIR__|$parser_script_dir|g;
$db_conf =~ s|__PERL__|$perlpath|g;

my $hncmd = 'hostname -s';
my $hostname = `$hncmd`;
chomp($hostname);

my $db_conf_file = "$make_script_dir/make_$hostname.conf";

open MAKE_CONFIG, ">$db_conf_file" or die "Could not create make config file $db_conf_file: $@\n";
print MAKE_CONFIG $db_conf;
close MAKE_CONFIG;

# copy over all the files in db-update
system("cd db-update; find . | cpio -p $make_script_dir") == 0
	or die "Could not copy over the files in db-update to $make_script_dir: $!\n";

# copy over all parsers
system("cd plugin/scripts; find . | cpio -p $parser_script_dir") == 0
	or die "Could not copy over the files in plugin/scripts to $parser_script_dir: $!\n";

# web site still needs to be done

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
#
#
#my $make = `which make`;
#chomp($make);
#
#while (1) {
#	my $make_version;
#	
#	open M, "$make -v|";
#	if (<M> =~ m/GNU Make (\d.\d+)/) {
#		$make_version = $1;
#	}
#	close M;
#	
#	last if defined $make_version and $make_version >= 3.79;
#	
#	$make = &ask_for_string("Which make do you want to use (should be a GNU make > 3.79)", $make);
#}

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

sub mkdir_recursive {
	my $dir = shift;

	$dir =~ s|/+$||;	# strip off trailing slash

	system("mkdir", "-p", $dir);

	return if -d $dir;	# if it exists we're done
		
	eval {
		print "Trying to create directory with sudo and mkdir\n";
		my @args = ("sudo", "mkdir", "-p", $dir);
		system(@args) == 0 or die "system @args failed: $?";
		chown $REAL_USER_ID, $dir;
		chmod 0755, $dir;
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