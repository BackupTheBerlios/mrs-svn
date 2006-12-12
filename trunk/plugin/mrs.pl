#!/usr/bin/perl
# 
# Copyright (c) 2005
#      CMBI, Radboud University Nijmegen. All rights reserved.
#
# This code is derived from software contributed by Maarten L. Hekkelman
# and Hekkelman Programmatuur b.v.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the Radboud University
#        Nijmegen and its contributors.
# 4. Neither the name of Radboud University nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

use strict;
use English;
use warnings;
use Time::HiRes qw(usleep ualarm gettimeofday tv_interval);
use MRS;
use Data::Dumper;

use Getopt::Std;

$| = 1;

# capture the commandline parameters

my $action = shift;
$action = '' unless defined $action;

my %opts;

if ($action eq 'create')
{
	getopts('d:s:vp:P:b:w:r:', \%opts);
	
	my $db = $opts{d} or &Usage();
	
	$MRS::VERBOSE = 1 if $opts{v};

	&Create($db, $opts{'s'}, $opts{p}, $opts{P}, $opts{b}, $opts{w}, $opts{r});
}
elsif ($action eq 'merge')
{
	getopts('d:m:P:lvs:', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	if ($opts{d} and $opts{P})
	{
		my $db = $opts{d} or &Usage();
		my $cnt = $opts{P} or &Usage();
	
		&Merge($db, $opts{'s'}, $cnt, $opts{l});
	}
	elsif ($opts{'m'})
	{
		&Merge2($opts{'m'}, $opts{'s'}, $opts{l});
	}
	else {
		&Usage();
	}
}
elsif ($action eq 'query')
{
	getopts('d:q:vr:', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();
	my $q = $opts{'q'};
	my $r = $opts{'r'};
	
	&Query($db, $q, $r);
}
elsif ($action eq 'entry')
{
	getopts('d:e:v', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();
	my $e = $opts{'e'};
	
	&Entry($db, $e);
}
elsif ($action eq 'blast')
{
	getopts('d:q:vi:', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();
	my $q = $opts{'q'};
	my $i = $opts{i} or &Usage();
	
	&Blast($db, $q, $i);
}
elsif ($action eq 'info')
{
	getopts('d:v', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();

	&Info($db);
}
elsif ($action eq 'dict')
{
	getopts('d:i:n:l:v', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();
	my $ix = $opts{i} or &Usage();

	&Dict($db, $ix, $opts{n}, $opts{l});
}
elsif ($action eq 'dump_index')
{
	getopts('d:i:n:Dl:', \%opts);

	my $db = $opts{d} or &Usage();
	my $ix = $opts{i} or &Usage();
	
	&Dump($db, $ix, $opts{n}, $opts{D}, $opts{l});
}
elsif ($action eq 'spellcheck')
{
	getopts('d:k:v', \%opts);

	$MRS::VERBOSE = 1 if $opts{v};

	my $db = $opts{d} or &Usage();
	my $kw = $opts{k} or &Usage();

	&SpellCheck($db, $kw);
}
elsif ($action eq 'version')
{
	getopts('d:', \%opts);

	my $db = $opts{d} or &Usage();

	&Version($db);
}
else
{
	&Usage();
}

sub Usage()
{
	my $usage=<<END;
Usage:

    mrs create -d databank [-s script] [-v] [-p part -P total]
    
        -d      databank name (or path)
        -s      script to use (if other than default for databank)
        -p      part number for this part
        -P      total number of parts
        -b		max weight bit count (default is 5)
        -w      stopword file to use
        -v      verbose

    mrs merge [-d databank -P total|-m databank [-s script]] [-v]
    
        -d      databank name
        -P      total number of parts
        -m      databank to merge, parts are defined by script
        -s      script to use with -m option (if other than default for databank)
        -l      link data instead of copying it
        -v      verbose

    mrs blast -d databank [-q 'mrs query'] [-v] -i query_file
	
        -d      databank name
        -q      optional MRS query to limit search space
        -i      query file in fasta format
        -v      verbose
    
    mrs info -d databank

        -d      databank name
       
    mrs dict -d databank -i "indexa[:indexb[:indexc]]]" [-n min_occur] [-l min_word_length]

        -d      databank name
        -i      index name(s) separated by colons
        -n      minimum number of document with key
        -l      minimal word length
        -v      verbose
       
    mrs dump_index -d databank -i index [-n min_occur] [-d] [-l min_word_length]

        -d      databank name
        -i      index name
        -n      minimum number of document with key
        -D      no leading digit in key name
        -l      minimal word length

    mrs entry -d databank -e entry
	
        -d      databank name
        -e      entry ID
        -v      verbose
    
END
		print $usage;
		exit(1);
}

sub LoadParser
{
	my ($script, $db) = @_;

	my $script_dir = $ENV{MRS_SCRIPT_DIR};
	my $raw_dir = $ENV{MRS_RAW_DIR} . $db;

	$script_dir	= substr($script_dir, 7)	if (substr($script_dir, 0, 7) eq 'file://');
	$raw_dir	= substr($raw_dir, 7)		if (substr($raw_dir, 0, 7) eq 'file://');
	
	push @INC, $script_dir;
	require "$script.pm";

	my $parser = "MRS::Script::${script}";
	
	# Now we can create the parser object
	
	my $p = new $parser(raw_dir => $raw_dir, db => $db) or die "Failed to create parser for $script\n";

	return $p;
}

sub Create()
{
	my ($db, $script, $partNr, $partCount, $weight_bit_count, $stopwordsfile, $rawfiles) = @_;
	
	$script = $db unless defined $script;

	die 'part number and total part count should both be specified'
		unless ((defined $partNr) == (defined $partCount));
	die 'partNr incorrect, should be 1 <= partNr <= total'
		if (defined $partNr and ($partNr < 1 or $partNr > $partCount));
	$weight_bit_count = 5 unless defined $weight_bit_count;
	
	# define some globals
	
	my $data_dir = $ENV{MRS_DATA_DIR};
	$data_dir	= substr($data_dir, 7)		if (substr($data_dir, 0, 7) eq 'file://');
	
	my $p = &LoadParser($script, $db);

	# Set the MRS globals before creating an MRS object

	$MRS::COMPRESSION = $p->{compression}						if defined $p->{compression};
	$MRS::COMPRESSION_LEVEL = $p->{compression_level}			if defined $p->{compression_level};
	$MRS::COMPRESSION_DICTIONARY = $p->{compression_dictionary}	if defined $p->{compression_dictionary};
	$MRS::WEIGHT_BIT_COUNT = $weight_bit_count;

	my $cmp_name = $db;
	$cmp_name .= "-$partNr" if defined $partNr;

	my $fileName = "$data_dir/$cmp_name.cmp";
	my $exists = -f $fileName;

	my $mrs = MRS::MDatabank::Create($fileName, $p->meta,
		$p->name, $p->version, $p->url, $script, $p->section) or
			die "Could not create new databank $db: " . &MRS::errstr();

	$p->{mrs} = $mrs;

	# protect the new born file
	chmod 0400, "$data_dir/$cmp_name.cmp" if not $exists;
	
	# get the stop words list
	
	if (defined $stopwordsfile)
	{
		my @stopWords = &ReadStopWords($stopwordsfile);
		$mrs->SetStopWords(\@stopWords);
	}

	# Ask the parser object for the list of files to process
	
	my @raw_files;
	if ($rawfiles) {
		@raw_files = split(m/,/, $rawfiles);
	}
	else {
		@raw_files = $p->raw_files;
	}
	
	# and the version for this db
	
	if (defined $partNr)
	{
		my $fileCount = scalar @raw_files;
		
		die 'not enough raw files to create all parts'
			unless $fileCount >= $partCount;

		my $n = sprintf('%.0d', $fileCount / $partCount); #/

		my $offset = $n * ($partNr - 1);
		my $length = $n;
		$length = $fileCount - ($n * ($partCount - 1)) if ($partNr == $partCount);
		
		if ($MRS::VERBOSE)
		{
			print "processing $fileCount files in $partCount parts, using $n files per part\n";
			print "now processing part $partNr containing $length files\n";
		}

		@raw_files = splice(@raw_files, $offset, $length);
	}

	die "No files to process!\n" unless scalar(@raw_files);
	
	my $n = 1;
	my $m = scalar @raw_files;
	
	foreach my $r (@raw_files)
	{
		SetProcTitle(sprintf("[%d/%d] MRS: '%s'", $n++, $m, $r));
		
		print "$r\n" if $MRS::VERBOSE;
		open IN, $r;
		$p->parse(*IN, $MRS::VERBOSE, $db, $r);
		close IN;
	}
	
	print "Parsing done, creating index... ";
	SetProcTitle("MRS: Parsing done, creating index... ");

	$mrs->Finish(1);	# and please create the weighted alltext index
	print "done!\n";
}

sub Merge()
{
	my ($db, $script, $cnt, $link) = @_;
	
	$script = $db unless defined $script;

	my $p = &LoadParser($script, $db);
	
	my $data_dir = $ENV{MRS_DATA_DIR};
	$data_dir	= substr($data_dir, 7)		if (substr($data_dir, 0, 7) eq 'file://');

	my @parts;
	foreach my $n (1 .. $cnt)
	{
		my $part = new MRS::MDatabank("$db-$n.cmp")
			or die "Could not find part $n: " . &MRS::errstr() . "\n";
		
		push @parts, $part;
	}
	
	MRS::MDatabank::Merge("$data_dir/$db.cmp", \@parts, $link ? 0 : 1,
		$p->name, $p->url, $script, $p->section);
}

sub Merge2()
{
	my ($db, $script, $link) = @_;
	
	# define some globals
	
	my $data_dir = $ENV{MRS_DATA_DIR};
	$data_dir	= substr($data_dir, 7)		if (substr($data_dir, 0, 7) eq 'file://');
		
	# now load the MRS plugin for this databank
	
	$script = $db unless defined $script;
	
	my $p = &LoadParser($script, $db);

	my $merge_databanks = $p->merge($db);
	
	die "No set of merge databanks defined in script $script" unless ref($merge_databanks) eq 'ARRAY';

	# define some globals

	my @parts;
	foreach my $p (@{$merge_databanks})
	{
		my $part = new MRS::MDatabank($p)
			or die "Could not find databank $p: " . &MRS::errstr() . "\n";
		
		push @parts, $part;
	}
	
	MRS::MDatabank::Merge("$data_dir/$db.cmp", \@parts, $link ? 0 : 1,
		$p->name, $p->url, $script, $p->section);
}

sub Query()
{
	my ($db, $q, $r) = @_;
	
	my $d = new MRS::MDatabank($db);
	
	my $s = $d->Find($q, 1);

#	my $rq = $d->RankedQuery('__ALL_TEXT__');
#		
#	foreach my $w (split(m/\s+/, $r)) {
#		$rq->AddTerm($w, 1);
#	}
#		
#	my $s = $rq->Perform;

	if ($s)
	{
		print "Hits for '$q' in $db\n";
		my $id;
		while (defined ($id = $s->Next))
		{
			print "$id\n";
		}
	}
	else
	{
		print "No hits found for '$q' in $db\n";
	}
}

sub Entry()
{
	my ($db, $entry) = @_;
	
	my $d = new MRS::MDatabank($db);
	
	print $d->Get($entry) . "\n\n";
}

sub Blast()
{
	my ($db, $q, $i) = @_;
	
	open INPUT, "<$i";
	my $line = <INPUT>;
	die "Not a valid fasta file" unless $line =~ /^>\S+/;
	
	my $input;
	while ($line = <INPUT>)
	{
		chomp($line);
		$line =~ s/\s//g;
		$input .= $line;
	}
	close INPUT;

	my $d = new MRS::MDatabank($db);
	my ($s, $hits, $hsps);
	
	if ($q)
	{
		$s = $d->Find($q);
		$hits = $s->Blast($input);
	}
	else
	{
		$hits = $d->Blast($input);
	}
	
	while (my $hsps = $hits->Next)
	{
		my $id = $hits->Id;
		
		if ($hsps->Next)
		{
			my $bitScore = $hsps->BitScore;
			print "$id\t$bitScore\n";
		}
	}
}

sub printNumber
{
	my $n = shift;

	1 while $n =~ s/(\d)(\d\d\d)(?!\d)/$1,$2/;
	
	return $n;
}

sub Info()
{
	my ($db) = @_;
	
	my $m = new MRS::MDatabank($db);

	my %index_types = (
		'text' => 'Text',
		'valu' => 'Value',
		'date' => 'Date',
		'nmbr' => 'Number',
		'wtxt' => 'Weighted',
	);

	print "Name: $db\n";
#	print 'Entries: ' . &printNumber($m->Count) . "\n";
	
	my $i = $m->Indices;
	
	die "No indices in databank\n" unless defined $i;
	
	while (my $ix = $i->Next)
	{
		my $type = $index_types{$ix->Type};
		my $name = $ix->Code;
		my $entries = &printNumber($ix->Count);

format Formaat1 =
@<<<<<<<<<<<<<<<<  @<<<<<<<  @>>>>>>>>>>>>>>>
$name,             $type,   $entries
.
		
		$FORMAT_NAME = 'Formaat1';
		write;
	}
	
	print "\n\n";
	$m->DumpInfo;
}

sub Dump()
{
	my ($db, $ix, $min_occurrence, $no_leading_digit, $min_wordlength) = @_;
	my $m = new MRS::MDatabank($db);

#	$m->DumpIndex($ix);
#exit;

	$min_occurrence = 1 unless defined $min_occurrence;
	$no_leading_digit = 0 unless defined $no_leading_digit;
	$min_wordlength = 1 unless defined $min_wordlength;

	my $i = $m->Indices or die "No indices in databank\n";

	print "dumping keys for index $ix\n";
	
	while (my $indx = $i->Next)
	{
		next if $indx->Code ne $ix;
		
		my $k = $indx->Keys or die "Index has no keys (???)\n";
		
		my ($key, $count);

format formaat2 =
@>>>>>>>>> @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$count, $key
.
		
		$FORMAT_NAME = 'formaat2';
		
		while (defined ($key = $k->Next))
		{
#			next if defined $min_wordlength and length($key) < $min_wordlength;
#			next if $no_leading_digit and $key =~ /^\d/;
			
			$count = $m->CountForKey($ix, $key);
			
#			next if defined $min_occurrence and $count < $min_occurrence;
			
			write;
		}

		last;
	}
}

sub Dict()
{
	my ($db, $ix, $min_occurrence, $min_wordlength) = @_;
	my $m = new MRS::MDatabank($db);
	
	$min_occurrence = 1 unless defined $min_occurrence;
	$min_wordlength = 1 unless defined $min_wordlength;
	
	$m->CreateDictionary($ix, $min_occurrence, $min_wordlength);
}

sub SpellCheck()
{
	my ($db, $kw) = @_;

	my $m = new MRS::MDatabank($db);
	
	if (my $i = $m->SuggestCorrection($kw))
	{
		while (my $s = $i->Next)
		{
			print "$s\n";
		}
	}
}

sub Version()
{
	my $db = shift;
	my $m = new MRS::MDatabank($db);

	print $m->GetVersion() . "\n";
}

sub ReadStopWords()
{
	my $stopwordsfile = shift;
	my %sw;
	
	if (open(SW, "<$stopwordsfile"))
	{
		while (my $line = <SW>)
		{
			if ($line =~ /^(\S+)/)
			{
				foreach my $p (split(m/'/, $1))
				{
					my $w = $p;
					$w =~ s/'.+$//;
					
					$sw{$w} = 1;
				}
			}
		}
		
		close SW;
	}
	else
	{
		warn("Could not open stop.txt\n");
	}
	
	return keys %sw;
}

sub SetProcTitle
{
	my $title = shift;
	
	eval {
		require Sys::Proctitle;
		Sys::Proctitle::setproctitle($title);
	};
	
	if ($@) {
		$PROGRAM_NAME = $title;
	}
}

package MRS::Script;

use Data::Dumper;
use File::stat;

# This is the base class for an MRS script

sub version
{
	my ($self, $version) = @_;
	
	if (defined $version) {
		$self->{version} = $version;
	}
	else {
		my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";
		my $db = $self->{db} or die "db is not defined\n";

		opendir DIR, $raw_dir;
		my @files = grep { -f("$raw_dir/$_") and $_ !~ /^\./ } readdir(DIR);
		closedir(DIR);
			
		my $date = 0;
		foreach my $f (@files)
		{
			my $sb = stat("$raw_dir/$f");
			
			if (defined $sb) {
				$date = $sb->mtime if $sb->mtime > $date;
			}
		}
		
		$date = stat($raw_dir)->mtime unless defined $date;
	
		$self->{version} = localtime $date;
	}
	
	return $self->{version};
}

sub url
{
	my ($self, $url) = @_;
	$self->{url} = $url if defined $url;
	return $self->{url};
}

sub name
{
	my ($self, $name) = @_;
	$self->{name} = $name if defined $name;
	return $self->{name};
}

sub section
{
	my ($self, $section) = @_;
	$self->{section} = $section if defined $section;
	return $self->{section};
}

sub meta
{
	my ($self, $meta) = @_;
	$self->{meta} = $meta if defined $meta;
	return $self->{meta};
}

sub merge
{
	my ($self, $db) = @_;
	die "No merge set defined for db $db\n" unless scalar($self->{merge}->{$db});
	return $self->{merge}->{$db};
}

sub raw_files
{
	my ($self) = @_;
	
	my $raw_files = $self->{raw_files};
	$raw_files = qr/.*/ unless defined $raw_files;
	
	my $raw_dir = $self->{raw_dir};

	opendir DIR, $raw_dir or die "Could not open raw dir $raw_dir\n";
	my @raw_files = grep { -e "$raw_dir/$_" and $_ =~ m/$raw_files/ } readdir(DIR);
	closedir DIR;
	
	return map {
		if ($_ =~ m/\.(gz|Z)$/) {
			"gunzip -c $raw_dir/$_ |";
		}
		else {
			"<$raw_dir/$_";
		}
	} @raw_files;
}

1;
