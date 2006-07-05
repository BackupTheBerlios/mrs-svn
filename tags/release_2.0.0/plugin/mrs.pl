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

use Getopt::Std;

$| = 1;

# capture the commandline parameters

my $action = shift;

my %opts;

if ($action eq 'create')
{
	getopts('d:s:vp:P:b:w:', \%opts);
	
	my $db = $opts{d} or &Usage();
	
	&Create($db, $opts{'s'}, $opts{v}, $opts{p}, $opts{P}, $opts{b}, $opts{w});
}
elsif ($action eq 'merge')
{
	getopts('d:P:lv', \%opts);

	my $db = $opts{d} or &Usage();
	my $cnt = $opts{P} or &Usage();
	
	&Merge($db, $opts{v}, $cnt, $opts{l});
}
elsif ($action eq 'weights')
{
	getopts('d:i:v', \%opts);

	my $db = $opts{d} or &Usage();
	my $ix = $opts{i} or &Usage();
	
	&Weights($db, $opts{v}, $ix);
}
elsif ($action eq 'query')
{
	getopts('d:q:vr:', \%opts);

	my $db = $opts{d} or &Usage();
	my $q = $opts{'q'};
	my $r = $opts{'r'};
	
	&Query($db, $opts{v}, $q, $r);
}
elsif ($action eq 'entry')
{
	getopts('d:e:v', \%opts);

	my $db = $opts{d} or &Usage();
	my $e = $opts{'e'};
	
	&Entry($db, $e, $opts{v});
}
elsif ($action eq 'blast')
{
	getopts('d:q:vi:', \%opts);

	my $db = $opts{d} or &Usage();
	my $q = $opts{'q'};
	my $i = $opts{i} or &Usage();
	
	&Blast($db, $opts{v}, $q, $i);
}
elsif ($action eq 'info')
{
	getopts('d:v', \%opts);

	my $db = $opts{d} or &Usage();

	&Info($db, $opts{v});
}
elsif ($action eq 'dict')
{
	getopts('d:i:n:l:v', \%opts);

	my $db = $opts{d} or &Usage();
	my $ix = $opts{i} or &Usage();

	&Dict($db, $ix, $opts{n}, $opts{l}, $opts{v});
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

	my $db = $opts{d} or &Usage();
	my $kw = $opts{k} or &Usage();

	&SpellCheck($db, $kw, $opts{v});
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

    mrs merge -d databank -P total [-v]
    
        -d      databank name
        -P      total number of parts
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

sub Create()
{
	my ($db, $script, $verbose, $partNr, $partCount, $weight_bit_count, $stopwordsfile) = @_;
	
	$script = $db unless defined $script;
	$verbose = 0 unless defined $verbose;
	die 'part number and total part count should both be specified'
		unless ((defined $partNr) == (defined $partCount));
	die 'partNr incorrect, should be 1 <= partNr <= total'
		if (defined $partNr and ($partNr < 1 or $partNr > $partCount));
	$weight_bit_count = 5 unless defined $weight_bit_count;
	
	# define some globals
	
	my $script_dir = $ENV{MRS_SCRIPT_DIR};
	my $data_dir = $ENV{MRS_DATA_DIR};
	my $raw_dir = $ENV{MRS_RAW_DIR} . $db;
	
	$script_dir	= substr($script_dir, 7)	if (substr($script_dir, 0, 7) eq 'file://');
	$data_dir	= substr($data_dir, 7)		if (substr($data_dir, 0, 7) eq 'file://');
	$raw_dir	= substr($raw_dir, 7)		if (substr($raw_dir, 0, 7) eq 'file://');
	
	# now load the MRS plugin for this databank
	
	push @INC, $script_dir;
	my $parser = "${script}::parser";
	require "$script.pm";
	
	# Set the MRS globals before creating an MRS object
	
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output
	$MRS::COMPRESSION = $parser::COMPRESSION
		if defined $parser::COMPRESSION;
	$MRS::COMPRESSION_LEVEL = $parser::COMPRESSION_LEVEL
		if defined $parser::COMPRESSION_LEVEL;
	$MRS::COMPRESSION_DICTIONARY = $parser::COMPRESSION_DICTIONARY
		if defined $parser::COMPRESSION_DICTIONARY;
	$MRS::WEIGHT_BIT_COUNT = $weight_bit_count;
	
	my $cmp_name = $db;
	$cmp_name .= "-$partNr" if defined $partNr;

	my $fileName = "$data_dir/$cmp_name.cmp";
	my $exists = -f $fileName;
	
	my $mrs = MRS::MDatabank::Create($fileName)
		or die "Could not create new databank $db: " . &MRS::errstr();

	# protect the new born file
	chmod 0400, "$data_dir/$cmp_name.cmp" if not $exists;
	
	# get the stop words list
	
	if (defined $stopwordsfile)
	{
		my @stopWords = &ReadStopWords($stopwordsfile);
		$mrs->SetStopWords(\@stopWords);
	}

	# Now we can create the parser object
	
	my $p = new $parser(mrs => $mrs);
	
	# Ask the parser object for the list of files to process
	
	my @raw_files = $p->raw_files($raw_dir, $db);
	
	# and the version for this db
	
	eval {
		my $vers = $p->version($raw_dir, $db);
		$mrs->SetVersion($vers);
	};
	
	if ($@) {
		print "$@\n";
	}
	
	if (defined $partNr)
	{
		my $fileCount = scalar @raw_files;
		
		die 'not enough raw files to create all parts'
			unless $fileCount >= $partCount;

		my $n = sprintf('%.0d', $fileCount / $partCount); #/

		my $offset = $n * ($partNr - 1);
		my $length = $n;
		$length = $fileCount - ($n * ($partCount - 1)) if ($partNr == $partCount);
		
		if ($verbose)
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
		
		print "$r\n" if $verbose;
		open IN, $r;
		$p->parse(*IN, $verbose, $db, $r);
		close IN;
	}
	
	print "Parsing done, creating index... ";
	SetProcTitle("MRS: Parsing done, creating index... ");

	$mrs->Finish(1);	# and please create the weighted alltext index
	print "done!\n";
}

sub Merge()
{
	my ($db, $verbose, $cnt, $link) = @_;
	
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output

	# define some globals
	
	my $data_dir = $ENV{MRS_DATA_DIR};
	$data_dir	= substr($data_dir, 7)		if (substr($data_dir, 0, 7) eq 'file://');

	my @parts;
	foreach my $n (1 .. $cnt)
	{
		my $part = new MRS::MDatabank("$db-$n.cmp")
			or die "Could not find part $n: " . &MRS::errstr() . "\n";
		
		push @parts, $part;
	}
	
	MRS::MDatabank::Merge("$data_dir/$db.cmp", \@parts, not $link);
}

sub Query()
{
	my ($db, $verbose, $q, $r) = @_;
	
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output
	
	my $d = new MRS::MDatabank($db);
	
#	my $s = $d->Find($q, 1);
	
		my $rq = $d->RankedQuery('*alltext*');
		
		foreach my $w (split(m/\s+/, $r)) {
			$rq->AddTerm($w, 1);
		}
		
	my $s = $rq->Perform;


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
	my ($db, $entry, $verbose) = @_;
	
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output
	
	my $d = new MRS::MDatabank($db);
	
	print $d->Get($entry) . "\n\n";
}

sub Blast()
{
	my ($db, $verbose, $q, $i) = @_;
	
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output
	
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
	my ($db, $verbose) = @_;
	
	$MRS::VERBOSE = $verbose if defined $verbose;
	
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
	my ($db, $ix, $min_occurrence, $min_wordlength, $verbose) = @_;
	my $m = new MRS::MDatabank($db);
	
	$min_occurrence = 1 unless defined $min_occurrence;
	$min_wordlength = 1 unless defined $min_wordlength;
	
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output

	$m->CreateDictionary($ix, $min_occurrence, $min_wordlength);
}

sub SpellCheck()
{
	my ($db, $kw, $verbose) = @_;

	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output

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

sub Weights()
{
	my ($db, $verbose, $ix) = @_;
		
	$verbose = 0 unless defined $verbose;
	$MRS::VERBOSE = $verbose;	# increase to get more diagnostic output

	my $m = new MRS::MDatabank($db);
	
	$m->RecalcDocWeights($ix);
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
