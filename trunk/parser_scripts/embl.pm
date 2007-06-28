# MRS plugin for creating an EMBL db
#
# $Id: embl.pm 131 2006-08-10 12:02:09Z hekkel $
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

package MRS::Script::embl;

use strict;
use POSIX qw/strftime/;

our @ISA = "MRS::Script";

my %INDICES = (
	'id' => 'Identification',
	'ac' => 'Accession number',
	'co' => 'Contigs',
	'cc' => 'Comments and Notes',
	'dt' => 'Date',
	'pr' => 'Project',
	'de' => 'Description',
	'gn' => 'Gene name',
	'os' => 'Organism species',
	'og' => 'Organelle',
	'oc' => 'Organism classification',
	'ox' => 'Taxonomy cross-reference',
	'ref' => 'Any reference field',
	'dr' => 'Database cross-reference',
	'kw' => 'Keywords',
	'ft' => 'Feature table data',
	'sv' => 'Sequence version',
	'fh' => 'Feature table header',
	'topology'	=> 'Topology (circular or linear)',
	'mt' => 'Molecule type',
	'dc' => 'Data class',
	'td' => 'Taxonomic division',
	'length' => 'Sequence length'
);

my %NAMES = (
	embl			=>	'Nucleotide (EMBL)',
	embl_release	=>	'EMBL Release',
	embl_updates	=>	'EMBL Updates',
);

sub new
{
	my $invocant = shift;

	our %merge_databanks = (
		'embl'	=>	[ 'embl_release', 'embl_updates' ],
	);
	
	my $self = {
		url			=> 'http://www.ebi.ac.uk/embl/index.html',
		section		=> 'nucleotide',
		meta		=> [ 'title' ],
		merge		=> \%merge_databanks,
		indices		=> \%INDICES,
		@_
	};
	
	if (defined $self->{db})
	{
		$self->{name} = $NAMES{$self->{db}};
		$self->{name} = $self->{db} unless defined $self->{name};
		
		if ($self->{db} eq 'embl_release') {
			$self->{raw_files} = qr/\.dat\.gz$/;
		}
		elsif ($self->{db} eq 'embl_updates') {
			
			# tricky... we want only the update files that match our release version of EMBL
			# so we need to find out the version number for the release first:
			
			my $rdb = new MRS::MDatabank('embl_release')
				or die "EMBL Release is not available, no way to determine version, sorry\n";
				
			if ($rdb->GetVersion =~ m/Release (\d+) /) {
				my $vers = int($1) + 1;
				$self->{raw_files} = qr/r${vers}u\d+\.dat\.gz/;
				$self->{version} = "Updates from Release $vers";
			}
			else {
				die "Unable to fetch version string from embl_release\n";
			}
		}
	}
	
	return bless $self, "MRS::Script::embl";
}

sub version
{
	my ($self) = @_;

	my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";
	my $db = $self->{db} or die "db is not defined\n";

	my $vers;
	
	if ($db eq 'embl_release')
	{
		open RELNOTES, "<$raw_dir/relnotes.txt";
		
		while (my $line = <RELNOTES>)
		{
			if ($line =~ /^\s+(Release\s+(\d+).+)/) {
				$vers = $1;
				last;
			}
		}

		close RELNOTES;
	}
	elsif ($db eq 'embl_updates') {
		$vers = $self->{version};
	}

	die "Unknown db: $db" unless defined $vers;

	chomp($vers);

	return $vers;
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my %ignore = (
		'SQ'	=> 1,
		'FH'	=> 1,
		'XX'	=> 1,
		'SV'	=> 1,
		'AH'	=> 1,
	);
	
	my %months = (
		'JAN'	=> 1,
		'FEB'	=> 2,
		'MAR'	=> 3,
		'APR'	=> 4,
		'MAY'	=> 5,
		'JUN'	=> 6,
		'JUL'	=> 7,
		'AUG'	=> 8,
		'SEP'	=> 9,
		'OCT'	=> 10,
		'NOV'	=> 11,
		'DEC'	=> 12
	);
	
	my ($id, $doc, $m, $title);
	
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		$doc .= $line;

		chomp($line);

		if ($line eq '//')
		{
			$m->StoreMetaData('title', $title);
			
			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
			$title = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}(.+)/o)
		{
			my $fld = $1;
			my $text = $2;

			if ($fld eq 'ID')
			{
				my @flds = split(m/;\s*/, $text);

				$m->IndexValue('id', $flds[0]);
				$flds[1] =~ m/SV (\d+)/ && $m->IndexNumber('sv', $1);
				$m->IndexWord('topology', lc($flds[2]));
#				$m->IndexWord('mt', lc($flds[3]));
				$m->IndexWord('dc', lc($flds[4]));
				$m->IndexWord('td', lc($flds[5]));
				$flds[6] =~ m/(\d+)/ && $m->IndexNumber('length', $1);
			}
			elsif ($fld eq 'DE')
			{
				$m->IndexText('de', $text);

				$title .= ' ' if defined $title;
				$title .= $text;
			}
			elsif ($fld eq 'DT')
			{
				if ($text =~ m/(\d{2})-([A-Z]{3})-(\d{4})/) {
					my $date = sprintf('%4.4d-%2.2d-%2.2d', $3, $months{$2}, $1);
					
					eval { $m->IndexDate('date', $date); };
					
					warn $@ if $@;
				}
			}
			elsif (substr($fld, 0, 1) eq 'R')
			{
				$m->IndexText('ref', substr($line, 5));
			}
			elsif ($fld eq 'PR')
			{
				if ($text =~ m/Project:(\s+?);/) {
					$m->IndexWord('pr', $1);
				}
			}
			elsif ($fld eq 'FT')
			{
				# avoid indexing the sequences in the translation feature
				
				if ($text =~ m|/([^=]+)="(.+)"|)
				{
					$m->IndexText('ft', $2) if ($1 ne 'translation');
				}
				elsif ($line =~ m|/([^=]+)="(.+)|)
				{
					my $skip = $1 eq 'translation';
					
					$m->IndexText('ft', $2) unless $skip;
					
					while ($lookahead =~ /^FT\s+(.+)/ and substr($1, 0, 1) ne '/')
					{
						$m->IndexText('ft', $1) unless $skip;

						$doc .= $lookahead;
						
						$lookahead = <IN>;
					}
				}
			}
			elsif ($ignore{$fld} != 1)
			{
				$m->IndexText(lc($fld), $text);
			}
		}
	}
}

my @links_db_xref = (
	{
		match	=> qr|^(embl:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=embl&query=ac:$2"}, $2)'
	},
	{
		match	=> qr[^(swiss-prot|sptrembl|uniprot/swiss-prot|uniprot/trembl|UniProtKB/Swiss-Prot):(\S+)]i,
		result	=> '$1.":".$q->a({-href=>"$url?db=uniprot&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(taxon:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=taxonomy&id=$2"}, $2)'
	},
	{
		match	=> qr|^(GO; GO:)(\d+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=go&id=$2"}, $2)'
	},
	{
		match	=> qr|^(GOA:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=goa&query=acc:$2"}, $2)'
	},
	{
		match	=> qr|^(InterPro:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=interpro&id=$2"}, $2)'
	},
	{
		match	=> qr|^(Pfam:)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfamseed|pfama|pfamb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(PDB:)(\d\w\w\w)|i,
		result	=> '"PDB:".$q->a({-href=>"$url?db=pdb&id=$2"}, "$2")'
	},
);

my %links = (
	'EMBL'	=>	{
		match	=> qr|^(\S+?)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=embl&query=ac:$1"}, $1)'
	},

	'UNIPROT'	=> {
		match	=> qr[^(\S+)(?=;)]i,
		result	=> '$q->a({-href=>"$url?db=uniprot&query=ac:$1"}, $1)'
	},

	'PDB'		=> {
		match	=> qr|^(\d\w\w\w)|i,
		result	=> '$q->a({-href=>"$url?db=pdb&id=$1"}, $1)'
	},

	'HSSP'		=> {
		match	=> qr|^(\S+;\s)(\d\w\w\w)|i,
		result	=> '$1.$q->a({-href=>"$url?db=hssp&id=$2"}, $2)'
	},

	'GO'		=> {
		match	=> qr|^(GO:)(\d+)|i,
		result	=> '$q->a({-href=>"$url?db=go&id=$2"}, $2)'
	},

	'Pfam'		=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=pfamseed|pfama|pfamb&query=ac:$1"}, $1)'
	},

	'InterPro'	=> {
		match	=> qr|^(\S+)(?=;)|i,
		result	=> '$q->a({-href=>"$url?db=interpro&id=$1"}, $1)'
	},

	'MIM'		=> {
		match	=> qr|^(\d+)|i,
		result	=> '$q->a({-href=>"$url?db=omim&id=$1"}, $1)'
	},

	'GDB'		=> {
		match	=> qr|^(\S+?)\.|,
		result	=> '$q->a({-href=>"http://www.gdb.org/gdb-bin/genera/accno?accessionNum=$1"}, $1)'
	}
);

sub print_ref
{
	my ($q, $ref) = @_;
	my @rr;

	foreach my $rx (@{$ref->{rx}})
	{
		my ($name, $ident) = split(m/[=:;]\s*/, $rx);
		
		$ident =~ s/\.$//;
		
		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?cmd=Retrieve&db=PubMed&list_uids=$ident&dopt=Abstract"}, $ident)
			if (lc $name eq 'medline');

		$ident = $q->a({-href=>"http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=pubmed&cmd=search&term=$ident"}, $ident)
			if (lc $name eq 'pubmed');
		
		$ident = $q->a({-href=>"http://dx.doi.org/$ident"}, $ident)
			if ($name eq 'DOI');
		
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, $name), $q->td($ident));
	}
	
	if ($ref->{rp})
	{
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Position'), $q->td(lc $ref->{rp}));
	}
	
	my @rc = split(m/;\s*/, $ref->{rc});
	foreach my $rc (@rc)
	{
		push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Comment'), $q->td(lc $rc));
	}

	push @rr, $q->Tr($q->td({-class=>'label', -width=>'20%'}, 'Reference Group'), $q->td($ref->{rg}))
		if defined $ref->{rg};
	
	my @l;
	push @l, $ref->{ra};
	push @l, $q->strong($ref->{rt});
	push @l, $ref->{rl};
	push @l, '<br/>';
	push @l, $q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @rr) if scalar @rr;
	
	return join(" ", @l);
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	my @rows;
	my (%ref, @ref_rows);
	
	my @lines = split(m/\n/, $text);
	my (@os, @oc, $og, @ox);
	my $kw_ix;
	
	my $lookahead = shift @lines;
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = shift @lines;
		
		# first block, the entry information
		if ($line =~ /^ID\s+(\S+)/)
		{
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Entry information'));
			push @rows, $q->Tr($q->td({-width=>'20%'}, 'Entry name'), $q->td($q->strong($1)));
		}
		elsif ($line =~ /^AC\s+(.+)/)
		{
			my @acc;
			
			for (;;)
			{
				push @acc, split(m/;\s*/, $1);
				
				last unless $lookahead =~ /^AC(.+)/;
				$lookahead = shift @lines;
			}

			push @rows, $q->Tr($q->td('Primary accession'), $q->td($q->strong(shift @acc)));
			push @rows, $q->Tr($q->td('Secondary accessions'), $q->td(join(' ', @acc)))
				if (scalar @acc);
		}
		elsif ($line =~ /^SV\s+(\S+)/)
		{
			push @rows, $q->Tr($q->td('Sequence Version'), $q->td($1));
		}
		elsif ($line =~ /^DT\s+(\d{2})-([A-Z]{3})-(\d{4}) \(Rel. (\d+), (.+?)(, Version (\d+))?\)/)
		{
			my %months = ( 'JAN' => 0, 'FEB' => 1, 'MAR' => 2, 'APR' => 3, 'MAY' => 4, 'JUN' => 5, 
				 'JUL' => 6, 'AUG' => 7, 'SEP' => 8, 'OCT' => 9, 'NOV' => 10, 'DEC' => 11);
			
			my $release = $4;
			my $desc = $5;
			my $date = strftime("%B %Y", 0, 0, 0, $1, $months{$2}, $3 - 1900);
			my $version = $7;
			
			my $s = "Release $release, $date";
			$s .= " (Version $version)" if defined $version;
			
			push @rows, $q->Tr($q->td($desc), $q->td($s));
		}

		# second block, name and origin
		
		elsif ($line =~ /^DE\s+(.+)/)
		{
			my $de = $1;
			
			while ($lookahead =~ /DE\s+(.+)/)
			{
				$de .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Description'));
			push @rows, $q->Tr($q->td('Description'), $q->td($de));
			
			$kw_ix = scalar @rows;
		}

		elsif ($line =~ /^KW\s+(.+?)$/ and $1 ne '.')
		{
			my @kw = split(m/;\s*/, $1);
			
			while ($lookahead =~ /^KW\s+(.+?)$/)
			{
				push @kw, split(m/;\s*/, $1);
				$lookahead = shift @lines;
			}

			my @kwr;
			push @kwr, $q->Tr($q->td('Keywords'), $q->td(join('; ', @kw)));
			splice(@rows, $kw_ix, 0, @kwr);
		}

		elsif ($line =~ /^OS\s+(.+?)(\.|,( and)?)?$/)
		{
			push @os, $1;
		}
		elsif ($line =~ /^OC\s+(.+?)(\.|;)?$/)
		{
			push @oc, split(m/;\s*/, $1);
		}
		elsif ($line =~ /^OG\s+(.+)/)
		{
			$og .= ';' if $og;
			$og .= $1;
		}
		
		# reference part
		
		elsif ($line =~ /^RN\s+\[(\d+)\]/)
		{
			my $nr = $1;
			
			# first flush the O part
			if (@os)
			{
#				die "mismatch OS " . scalar @os . " OX " . scalar @ox  if scalar @os != scalar @ox;
				
				my @osr;
				
				for (my $i = 0; $i < scalar @os; ++$i)
				{
					my $ox = $ox[$i];
					my $os = $os[$i];
					
					if ($ox)
					{
						my $taxon_url = $q->a({-href=>"$url?db=taxonomy&id=$ox"}, $ox);
						$os = "$os (Taxon ID: $taxon_url)";
					}
					push @osr, $q->td($os);
				}
				
				push @rows, $q->Tr($q->td({-rowspan=>(scalar @osr)}, 'From'), shift @osr);
				push @rows, $q->Tr(\@osr) if scalar @osr;
				
				push @rows, $q->Tr($q->td('Encoded on'), $q->td($og)) if $og;
				
				push @rows, $q->Tr($q->td('Taxonomy'), $q->td(
					join("; ", map { $q->a({-href=>"$url?db=taxonomy&query=$_"}, $_) } @oc)));
				
				undef @os;
				undef $og;
				undef @ox;
				undef @oc;

				push @rows, $q->Tr($q->th({-colspan=>2}, 'References'));
			}
			
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			
			undef %ref;
			$ref{nr} = $nr;
		}
		
		elsif ($line =~ /^RP\s+(.+)/)
		{
			$ref{rp} .= "$1 ";
		}
		elsif ($line =~ /^RX\s+(.+)/)
		{
			my @rx = ();
			$ref{rx} = \@rx unless $ref{rx};
			push @{$ref{rx}}, $1;
		}
		elsif ($line =~ /^RC\s+(.+)/)
		{
			$ref{rc} .= "$1 ";
		}
		elsif ($line =~ /^RG\s+(.+)/)
		{
			$ref{rg} .= "$1 ";
		}
		elsif ($line =~ /^RA\s+(.+)/)
		{
			$ref{ra} .= "$1 ";
		}
		elsif ($line =~ /^RT\s+(.+?);?$/)
		{
			$ref{rt} .= "$1 ";
		}
		elsif ($line =~ /^RL\s+(.+?)\.?$/)
		{
			$ref{rl} .= "$1 ";
		}

		elsif ($line =~ /^XX/)
		{
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			undef %ref;
		}
		
		# cross reference
		
		elsif ($line =~ /^DR\s+(.+)/)
		{
			if (%ref)
			{
				push @rows, $q->Tr($q->td($ref{nr}), $q->td($q->div({-class=>'sub_entry'},
					&print_ref($q, \%ref))));
			}
			undef %ref;
			
			my @dr;
			my $last_db;

			push @rows, $q->Tr($q->th({-colspan=>2}, 'Cross-references'));
			
			for (;;)
			{
				my ($db, $ids) = split(m/;\s*/, substr($line, 5), 2);

				if ($db ne $last_db and scalar @dr)
				{
					
					push @rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
					push @rows, $q->Tr(\@dr) if scalar @dr;
					
					undef @dr;
				}
				
				if (my $l = $links{$db})
				{
					$ids =~ s/$l->{match}/$l->{result}/eegm;
				}
				
				push @dr, $q->td($ids);
				$last_db = $db;
				
				last if (substr($lookahead, 0, 2) ne 'DR');
				$line = $lookahead;
				$lookahead = shift @lines;
			}

			push @rows, $q->Tr($q->td({-rowspan=>(scalar @dr)}, $last_db), shift @dr);
			push @rows, $q->Tr(\@dr) if scalar @dr;
		}
		
		# comments
		
		elsif ($line =~ /^CC\s*(.+)/)
		{
			my $cc = $1;
			
			while ($lookahead =~ /CC\s+(.+)/)
			{
				$cc .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Comment'));
			push @rows, $q->Tr($q->td({-colspan=>2}, $cc));
		}
	
		# AH/AS	assembly information
		
		elsif ($line =~ m/^AH/)
		{
			#      0    5               41                   62             77
			#      AH   TPA_SPAN        PRIMARY_IDENTIFIER   PRIMARY_SPAN   COMP
			
			my @as;
			push @as, $q->Tr(
				$q->th({-width=>'10%'}, 'Primary seq.'),
				$q->th({-width=>'10%'}, 'Span in assembly'),
				$q->th({-width=>'80%'}, 'Span in primary sequence')
			);
			
			while ($lookahead =~ /AS\s+(.+?)/)
			{
				my $comp = '';
				$comp = '(Complement)' if length($lookahead) > 77 and substr($lookahead, 77, 1) eq 'c';
				
				push @as, $q->Tr(
					$q->td(substr($lookahead, 5, 35)),
					$q->td(substr($lookahead, 41, 20)),
					$q->td(substr($lookahead, 62, 14) . $comp)
				);
				
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Assembly information'));
			push @rows, $q->Tr($q->td({-colspan=>2}, $q->div({-class=>'sub_entry'},
					$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @as))));
		}
		
		# CO
		
		elsif ($line =~ /^CO\s*(.+)/)
		{
			my $co = $1;
			
			while ($lookahead =~ /CO\s+(.+)/)
			{
				$co .= " $1";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Constructed'));
			push @rows, $q->Tr($q->td('Contigs'), $q->td($co));
		}
		
		# Feature table
		
		elsif ($line =~ /^FT/)
		{
			my @ft;
			
			push @ft, $q->Tr(
				$q->th({-width=>'10%'}, 'Key'),
				$q->th({-width=>'10%'}, 'Location'),
				$q->th({-width=>'10%'}, 'Qualifier'),
				$q->th({-width=>'70%'}, 'Value'));
			
			for (;;)
			{
				my $key = substr($line, 5, 8);		$key =~ s/\s+$//;
				my $location = substr($line, 21);	$location =~ s/\s+$//;
				
				my (@fd, $f);
				
				while ($lookahead =~ m|^FT\s{10,}([^/\s].+)|)
				{
					$location .= " $1";
					$lookahead = shift @lines;
				}
				
				while ($lookahead =~ /^FT\s{10,}(.+)/)
				{
					my $nf = $1;
					
					if ($f and length($f) and $nf =~ m|/(\S+?)=| and $f =~ m|/(\S+?)=(.+)|)
					{
						# unfinished feature line... see if we can fix it

						push @fd, $q->td($1) . $q->td($2);
						$f =~ s|/(\S+?)=(.+)||;
					}
					
					$f .= " $nf";
					
					if ($f =~ m|/(\S+?)="(.+)"|)
					{
						my $fq = $1;	$fq =~ s/\s+$//;
						my $fv = $2;	$fv =~ s/\s+$//;
						
						if ($fq eq 'translation')
						{
							$fv =~ s/\s//g;
							$fv =~ s/(.{30})/$1\n/g;
							$fv = $q->div({-id=>'translation-box'}, $fv);
						}
						elsif ($fq eq 'db_xref')
						{
							foreach my $l (@links_db_xref)
							{
								$fv =~ s/$l->{match}/$l->{result}/eem;
							}							
						}
						elsif ($fq eq 'EC_number')
						{
							$fv = $q->a({-href=>"$url?db=enzyme&id=$fv"}, $fv); 
						}
						
						push @fd, $q->td($fq) . $q->td($fv);
						$f =~ s|/(\S+?)="(.+)"||;
					}

					if ($f =~ m|/(\S+?)=(\d+)|)
					{
						push @fd, $q->td($1) . $q->td($2);
						$f =~ s|/(\S+?)=(\d+)||;
					}
					
					$lookahead = shift @lines;
				}
				
				$location =~ s/,/, /g;
				
				if (scalar @fd > 1)
				{
					push @ft, $q->Tr(
						$q->td({-rowspan=>(scalar @fd)}, $key),
						$q->td({-rowspan=>(scalar @fd)}, $location),
						shift @fd);
				}
				else
				{
					push @ft, $q->Tr(
						$q->td($key),
						$q->td($location),
						shift @fd);
				}

				push @ft, $q->Tr(\@fd) if scalar @fd;
				
				last unless $lookahead =~ /^FT/;
				
				$line = $lookahead;
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Features'));
			push @rows, $q->Tr($q->td({-colspan=>2}, $q->div({-class=>'sub_entry'},
					$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @ft))));
		}
		
		elsif ($line =~ /^SQ\s*Sequence\s*(\d+)\s*BP;\s*(.+)/)
		{
			my $bp = $1;

			my $c = $2;
			my ($ac, $cc, $tc, $gc, $oc);
			
			$ac = $1 if $c =~ /(\d+)\s*A;/;
			$cc = $1 if $c =~ /(\d+)\s*C;/;
			$tc = $1 if $c =~ /(\d+)\s*T;/;
			$gc = $1 if $c =~ /(\d+)\s*G;/;
			$oc = $1 if $c =~ /(\d+)\s*Other;/;
			
			push @rows, $q->Tr($q->th({-colspan=>2}, 'Sequence information'));
			
			my $cnt = "Length: <strong>$bp</strong> BP";
			$cnt .= ", A Count: <strong>$ac</strong>" if $ac;
			$cnt .= ", C Count: <strong>$cc</strong>" if $cc;
			$cnt .= ", T Count: <strong>$tc</strong>" if $tc;
			$cnt .= ", G Count: <strong>$gc</strong>" if $gc;
			$cnt .= ", Other Count: <strong>$oc</strong>" if $oc;
			
			push @rows, $q->Tr($q->td({-colspan=>2}, $cnt));

			my $seq;
			while (not ($lookahead =~ m|//|))
			{
				$lookahead =~ s/^\s+//;
				
				$seq .= "$lookahead\n";
				$lookahead = shift @lines;
			}
			
			push @rows, $q->Tr($q->td({-colspan=>2}, $q->pre($seq)));
		}
		
	}

	return $q->div({-class=>'entry'},
		$q->table({-cellspacing=>'0', -cellpadding=>'0', -width=>'100%'}, @rows));

}

1;
