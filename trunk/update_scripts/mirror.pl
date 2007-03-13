#!perl

use strict;
use warnings;

use URI;
use Net::FTP;
use Getopt::Std;

my %opts;
my $recursive = 0;
my $dest = ".";
my $verbose = 1;
my $cleanup = 0;
my $include;
my $no_fetch = 0;
my $url;

$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('d:i:rcvn', \%opts);

$recursive = 1 if defined $opts{r};
$dest = $opts{d} if defined $opts{d};
$include = $opts{i} if defined $opts{i};
$verbose = 2 if defined $opts{v};
$cleanup = 1 if defined $opts{c};
$no_fetch = 1 if defined $opts{n};

$url = shift;
&HELP_MESSAGE() if not defined $url;

$| = 1;

my $ok = 0;

for (my $retry = 0; $ok == 0 and $retry < 5; ++$retry)
{
	print "Retry $retry...\n" unless $retry == 0;
	
	eval
	{
		&main($url);
		$ok = 1;
	};
	
	if ($@)
	{
		print "\n\nError in mirror.pl: $@\n";
	}
}

die "aborting\n" unless $ok;
exit;

sub VERSION_MESSAGE()
{
	print "mirror.pl 0.1 -- a lightweight ftp mirror tool by M.L. Hekkelman\n";
}

sub HELP_MESSAGE()
{
	&VERSION_MESSAGE();
	print "usage: mirror.pl [options] url\n";
	print "options:\n";
	print "    -d directory         destination directory (default is current)\n";
	print "    -i pattern           pattern defining the names of files to download\n";
	print "    -r                   recursive download\n";
	print "    -c                   clean up, remove files not found on remote side\n";
	print "    -n                   don't fetch or delete, just tell what would be done\n";
	print "    -v                   verbose mode\n";
	exit;
}

sub fetch($$$)
{
	my ($s, $remote_dir, $local_dir) = @_;

	print "Entering $remote_dir\n" if $verbose > 1;
	
	if (not chdir($local_dir))
	{
		mkdir($local_dir) or die "Could not create local directory $local_dir";
		chdir($local_dir) or die "Could not create local directory $local_dir";
	};

	my %localFiles;
	if ($cleanup)
	{
		opendir(DIR, ".") or die "can't opendir $local_dir: $!";
		%localFiles = map { $_ => 1 } grep { -f "$local_dir/$_" } readdir(DIR);
		closedir DIR;
	}
	
#	my @ls = $s->ls();
#	if (scalar(@ls) == 0 or 1)	# stupid FTP servers... 
#	{
		my @ls;
		foreach my $f ($s->dir())
		{
			if (($f =~ s/^.+?(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2}\s+\d\d\:\d\d\s(.+)/$2/oi) or
			    ($f =~ s/^.+?(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2}\s+\d{4}\s(.+)/$2/oi))
			{
				push @ls, $f;
			}
		}
#	}

	print "got listing\n" if $verbose > 1;
	
	foreach my $e (sort @ls)
	{
		next if $e eq "." or $e eq "..";

		my $path = "$remote_dir/$e";

		if ($e =~ m/(.+) -> (.+)/) {   # a symbolic link perhaps? treat as regular file...
			$path = "$remote_dir/$2";
			$e = $1;
		}
		
		my ($mdtm, $size);
		
		if (not defined $include or $e =~ /$include/)
		{
			$mdtm = $s->mdtm($path) or warn "Server does not support mdtm ($path)\n";
			$size = $s->size($path) if defined $mdtm;
		}

		if (defined $mdtm and defined $size)
		{
			my $file = "$local_dir/$e";
			my $fetch = 1;
			
			if (-f $file)
			{
				my @stat = stat($file);
				
				$fetch = ($stat[9] != $mdtm or $stat[7] != $size);
			}
			
			if ($fetch)
			{
				print "Fetching $e... " if $verbose > 1 or $no_fetch;
				print "$e\n" if $verbose == 1;
				
				if (not $no_fetch)
				{
					my $tmp = "$local_dir/.$e$$";
					$s->get($path, $tmp) or die "Could not fetch file $e: $!";
					unlink($file);
					rename($tmp, $file);
					utime $mdtm, $mdtm, $file;
					print "done" if $verbose > 1;
				}
			}
			elsif ($verbose > 1)
			{
				print "Up to date $e";
			}
			
			$localFiles{$e} = 0;

			print "\n" if $verbose > 1 or $no_fetch;
		}
		elsif ($recursive)
		{
			&fetch($s, "$remote_dir/$e", "$local_dir/$e")
				if $s->cwd($path);

			$s->cwd($remote_dir) or die "Could not change directory: $!";
		}
	}

	foreach my $r (keys %localFiles)
	{
		if ($localFiles{$r} == 1)
		{
			print "Deleting $r\n" if $verbose or $no_fetch;
			unlink "$local_dir/$r" if not $no_fetch;
		}
	}
}

sub main
{
	my $url = shift;
	my $uri = new URI($url) or die "not a valid url: $!";
	
	die "scheme not supported" if $uri->scheme ne 'ftp';
	
	my $host = $uri->host;
	my $port = $uri->port;
	$port = 21 if not defined $port;
	
	my $s = new Net::FTP($uri->host, Passive => 1, Port => $port)
		or die "Could not create FTP object";
	
	my $user = $uri->user;
	$user = 'ftp' if not defined $user;
	$s->login($user) or die "Could not login to server: $!";
	
	my $path = $uri->path;
	if (defined $path)
	{
		$path =~ s|^/||;
		print "cwd: $path\n" if $verbose > 1;
		$s->cwd($path) or die "Could not change directory: $!";
	}
	
	$path = $s->pwd();
	$s->binary or die "Could not change to binary mode";
	
	&fetch($s, $path, $dest);
}
