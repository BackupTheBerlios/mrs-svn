#!/usr/bin/perl -w
# 
# installation script for an MRS website.
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use English;
use Sys::Hostname;

$| = 1;	# flush stdout

# some globals

my $httpd;			# path to apache webserver
my $httpd_conf;		# path to apache webserver config file
my $site_dir;		# where to install the website
my $data_dir;		# where to store the data files
my $os;				# the OS we're installing on
my $cpu;			# the CPU we're using here

my %supported = (
	'Darwin_powerpc' => 'mrs-Darwin-powerpc.tgz',
	'Linux_i686' => 'mrs-Linux-i686.tgz'
);

my @mrs_files = (
	'pdbfinder2',
	'uniprot',
	'dssp',
	'hssp',
	'pdb'
);

# let's start with a sanity check

#die "This version of perl is not supported, please make sure you use 5.8.x\n"
#	unless $PERL_VERSION ge v5.8.0;

print "Not running as root, this will probably fail but we'll see how far we will get.\n\n"
	unless $EFFECTIVE_USER_ID == 0;

$os = `uname -s`; chomp($os);
$cpu = `uname -p`; chomp($cpu);

if (not defined $supported{"${os}_${cpu}"})
{
	print "Could not determine the correct OS and CPU type to use\n\n";

	while ($os ne 'Linux' and $os ne 'Darwin')
	{
		print "  (1) Linux\n";
		print "  (2) MacOS X\n";
		print "\nPlease choose your OS: ";
		my $os_c = <>; chomp($os_c);
		$os = 'Linux' if $os_c == 1;
		$os = 'Darwin' if $os_c == 2;
	}
	
	while ($cpu ne 'i686' and $cpu ne 'powerpc')
	{
		print "  (1) PowerPC\n";
		print "  (2) Intel\n";
		print "Please choose your CPU type: ";
		my $cpu_c = <>; chomp($cpu_c);
		$cpu = 'powerpc' if $cpu_c == 1;
		$cpu = 'i686' if $cpu_c == 2;
	}
	
	die "This combination of CPU and OS is not (yet) supported (${os}_${cpu})\n"
		unless defined $supported{"${os}_${cpu}"};
}

# First find out if we're allowed to run on this computer with this account:

$httpd = `which httpd`
	or die "\nError: Could not find apache.\nMake sure you have apache installed and run this script as root\n";
chomp($httpd);

# fetch the location of the apache configfile

my $httpd_root;

open FH, "$httpd -V|";
while (my $line = <FH>)
{
	if ($line =~ /-D HTTPD_ROOT="(.+)"/)
	{
		$httpd_root = $1;
	}

	if ($line =~ /-D SERVER_CONFIG_FILE="(.+)"/)
	{
		$httpd_conf = $1;
	}
}
close FH;

$httpd_conf = "$httpd_root/$httpd_conf"
	if (defined $httpd_conf and defined $httpd_root and substr($httpd_conf, 0, 1) ne '/');

die "The apache config file ($httpd_conf) cannot be edited. Run this script again as root.\n" unless (-w $httpd_conf);

print "\nUsing apache found at $httpd and its config file located at:\n$httpd_conf\n";

# read in the entire config file:

my $httpd_conf_data;
{
	local($/) = undef;
	
	open FH, "<$httpd_conf";
	$httpd_conf_data = <FH>;
	close FH;
}
die "Could not read $httpd_conf\n" unless defined $httpd_conf_data and length($httpd_conf_data);

# Now ask the user where to install the MRS website

my $DOCROOT = '/usr/local/www';
if ($httpd_conf_data =~ /^\s*documentroot\s*(.+)/im)
{
	$DOCROOT = $1;
	$DOCROOT =~ s/(['"])(.+)(\1)/$2/;
	$DOCROOT =~ s|/+$||;
}

my $WWW_USER = 'www';
if ($httpd_conf_data =~ /^\s*user\s+(.+)/im)
{
	$WWW_USER = $1;
	$WWW_USER =~ s/(['"])(.+)(\1)/$2/;
}

my $WWW_GROUP = 'www';
if ($httpd_conf_data =~ /^\s*group\s+(.+)/im)
{
	$WWW_GROUP = $1;
	$WWW_GROUP =~ s/(['"])(.+)(\1)/$2/;
}

print "\nWhere do you want to install the MRS website [$DOCROOT/mrs]: ";
$site_dir = <>;
chomp($site_dir);

$site_dir = "$DOCROOT/mrs" unless length($site_dir) > 0;

die "Could not create directory $site_dir\n"
	unless -d $site_dir or mkdir($site_dir);

die "Could not create directory $site_dir/htdocs\n"
	unless -d "$site_dir/htdocs" or mkdir("$site_dir/htdocs");

die "Could not create directory $site_dir/cgi-bin\n"
	unless -d "$site_dir/cgi-bin" or mkdir("$site_dir/cgi-bin");

# fetch the site's scripts, first try to use Net::FTP, fall back to wget

print "Fetching scripts and plugin\n";

my $pkg = $supported{"${os}_${cpu}"};
my $tmp = "/tmp/mrs_download_$$.tgz";

eval
{
	use Net::FTP;
	
	my $s = new Net::FTP('ftp.cmbi.ru.nl', Passive => 1)
		or die "Could not connect to ftp.cmbi.ru.nl\n";
	$s->login('ftp')
		or die "Could not login as user anonymous\n";
	$s->cwd('/pub/software/mrs')
		or die "Could not cd to /pub/software/mrs on ftp.cmbi.ru.nl\n";
	$s->binary
		or die "Could not change mode to binary\n";
	$s->get($pkg, $tmp)
		or die "Could not fetch file $pkg\n";
};

if ($@)
{
	print "Failed to fetch scripts using Net::FTP\n\n  $@\n\ntrying wget\n";
	
	system("wget -O $tmp ftp://ftp.cmbi.ru.nl/pub/software/mrs/$pkg");
}

die "Could not fetch $pkg" unless -f $tmp;

chdir($site_dir);
system("tar xvzf $tmp");
unlink($tmp);
system("chown -R $WWW_USER:$WWW_GROUP '$site_dir'");

die "Something went wrong extracting the archive\n"
	unless -f "$site_dir/cgi-bin/mrs.cgi";

# Now make sure the plugin actually works

print "Testing if we can load the MRS plugin... ";

chdir("$site_dir/cgi-bin");
eval "require MRS";

if ($@)
{
	print "Error\n\n";
	die "Apparently you cannot use this perl plugin\n" .
		"Please report this problem to m.hekkelman\@cmbi.ru.nl " .
		"and append the following data:\n\n" .
		$@;
}

print "OK\n\n";

# Now create a new settings file for mrs, we now need to know where to put
# the mrs data files

print "Where do you want to store the mrs data files? [/usr/local/mrs]: ";
$data_dir = <>;
chomp($data_dir);
$data_dir = '/usr/local/mrs' unless length($data_dir) > 0;

die "Could not create directory $data_dir\n"
	unless -d $data_dir or mkdir($data_dir);

my $settings;
open FH, "<mrs.conf" or die "Could not open mrs.conf file";
{
	local($/) = undef;
	$settings = <FH>;
}
close FH;

$settings =~ s/__DATA_DIR__/$data_dir/;

open FH, ">mrs.conf" or die "Could not create mrs settings file";
print FH $settings;
close FH;

# OK, so that's done. Now edit the apache config file.

print "\nNow we have to edit the apache config file to make mrs work\n";
print "The lines to be added are:\n\n";
print "ScriptAlias /mrs/cgi-bin/ \"$site_dir/cgi-bin/\"\n";
print "Alias /mrs/ \"$site_dir/htdocs/\"\n";
print "\nDo you want me to add them to $httpd_conf? [y/n]: ";
while (1)
{
	my $answer = lc(<>);
	chomp($answer);
	if (defined $answer and length($answer))
	{
		if ($answer eq 'y' or $answer eq 'yes')
		{
			open FH, ">>$httpd_conf" or die "Could not open $httpd_conf for editing\n";
			print FH "\n# Following lines added by $0\n";
			print FH "ScriptAlias /mrs/cgi-bin/ \"$site_dir/cgi-bin/\"\n";
			print FH "Alias /mrs/ \"$site_dir/htdocs/\"\n";
			close FH;

			print "Trying to restart apache... ";
			system("apachectl restart");

			last;
		}
		
		if ($answer eq 'n' or $answer eq 'no')
		{
			print "Please make sure you add these lines yourself\n";
			last;
		}
	}
	print "Please answer yes or no: ";
}

my $hostname = hostname();
print "\nMRS should now be accessible at http://$hostname/mrs/cgi-bin/mrs.cgi\n\n";

# And now fetch the first data files.

chdir($data_dir);

eval
{
	use Net::FTP;
	
	my $s = new Net::FTP('ftp.cmbi.ru.nl', Passive => 1)
		or die "Could not connect to ftp.cmbi.ru.nl\n";
	$s->login('ftp')
		or die "Could not login as user anonymous\n";
	$s->cwd('/pub/molbio/data/mrs')
		or die "Could not cd to /pub/molbio/data/mrs on ftp.cmbi.ru.nl\n";
	$s->binary
		or die "Could not change mode to binary\n";

	foreach my $db (@mrs_files)
	{
		my $file = "$db.cmp";
		
		my ($mdtm, $size);
		
		$mdtm = $s->mdtm($file);
		$size = $s->size($file) if defined $mdtm;

		if (defined $mdtm and defined $size)
		{
			my $fetch = 1;
			
			if (-f $file)
			{
				my @stat = stat($file);
				
				$fetch = ($stat[9] != $mdtm or $stat[7] != $size);
			}
			
			if ($fetch)
			{
				print "Downloading $file... ";
				
				my $tmp = ".$file$$";
				$s->get($file, $tmp) or die "Could not fetch file $file: $!";
				unlink($file);
				rename($tmp, $file);
				utime $mdtm, $mdtm, $file;
				print "done\n";
			}
			else
			{
				print "$file is already up to date\n";
			}
		}
	}
};

if ($@)
{
	print "Failed to fetch data files using Net::FTP\n\n  $@\n\ntrying wget\n";
	
	foreach my $db (@mrs_files)
	{
		system("wget ftp://ftp.cmbi.ru.nl/pub/molbio/data/mrs/$db.cmp");
	}
}

print "\n\nDone!\n";
