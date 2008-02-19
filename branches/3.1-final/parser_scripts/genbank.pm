# MRS plugin for creating a db
#
# $Id: genbank.pm 79 2006-05-30 08:36:36Z maarten $
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

package MRS::Script::genbank;

use strict;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;

	my %merge_databanks = (
		refseq		=>	[ 'refseq_release', 'refseq_updates' ],
		genbank		=>	[ 'genbank_release', 'genbank_updates' ],
	);

	my $self = {
		meta		=> [ 'title' ],
		merge		=> \%merge_databanks,
		section		=> 'genomic',
		raw_files	=> qr/\.gz$/,
		@_
	};
	
	my %NAME = (
		genbank				=> 'Genbank',
		refseq				=> 'REFSEQ',
	);
	
	my %URL = (
		genbank				=> 'http://www.ncbi.nlm.nih.gov/Genbank/GenbankOverview.html',
		refseq				=> 'http://www.ncbi.nlm.nih.gov/RefSeq/',
	);
	
	my %SECTION = (
		genbank				=> 'nucleotide',
		refseq				=> 'other',
	);
	
	if (defined $self->{db}) {

		my ($dbn, $sn) = split(m/_/, $self->{db});

		$self->{url} = $URL{$dbn};
		$self->{name} = $NAME{$dbn};
		
		$self->{name} .= " $sn" if defined $sn;

		if ($self->{db} eq 'genbank_release')
		{
			open RELDATE, "<$self->{raw_dir}/README.genbank";
			
			while (my $line = <RELDATE>)
			{
				if ($line =~ /GenBank Flat File Release/) {
					$self->{version} = $line;
					last;
				}
			}
			
			close RELDATE;
		}
	}

	return bless $self, "MRS::Script::genbank";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $id, $m, $done, $field, $prot, $title);
	
	$done = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;

	$lookahead = <IN>
		while (defined $lookahead and substr($lookahead, 0, 5) ne 'LOCUS');

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

			$doc = undef;
			$field = undef;
			$id = undef;
			$title = undef;
			$done = 0;
		}
		elsif (not $done)
		{
			my $key = substr($line, 0, 10);
			
			if ($key =~ /^\s*(\S.*?)\s*$/o)
			{
				$field = lc($1);
			}
			
			if ($field eq 'origin')
			{
				if ($prot)
				{
					my $seq = "";
					
					while ($lookahead and $lookahead =~ /^\s+\d+\s+(.+)/)
					{
						$seq .= $1;
						$lookahead = <IN>;
					}
					
					$seq =~ s/\s//g;
					$m->AddSequence($seq);
				}
				
				$done = 1;
				next;
			}
			elsif ($field eq 'reference')
			{
				$m->IndexText('reference', substr($line, 12));
				while (defined $lookahead and substr($lookahead, 0, 1) eq ' ')
				{
					$m->IndexText('reference', substr($lookahead, 12));
					$doc .= $lookahead;
					$lookahead = <IN>;
				}
			}
			elsif ($field eq 'features')
			{
				while (defined $lookahead and substr($lookahead, 0, 1) eq ' ')
				{
					my $fkey = substr($lookahead, 5, 15);
					my $ftxt = substr($lookahead, 21);
					chomp($ftxt);

					$doc .= $lookahead;
					$lookahead = <IN>;
					
					$m->IndexText('features', $fkey);

					if ($ftxt =~ m|/([^=]+)="(.+)"|o)
					{
						$m->IndexText('features', $2) if ($1 ne 'translation');
					}
					elsif ($ftxt =~ m|/([^=]+)="(.+)|o)
					{
						my $skip = $1 eq 'translation';

						$m->IndexText('features', $2) unless $skip;
						
						while (defined $lookahead and
							substr($lookahead, 0, 1) eq ' ' and
							substr($lookahead, 21, 1) ne '/')
						{
							$m->IndexText('features', substr($lookahead, 22)) unless $skip;
							
							$doc .= $lookahead;
							$lookahead = <IN>;
						}
					}
					else
					{
						$m->IndexText('features', $ftxt);
					}
				}
			}
			elsif (length($line) > 12)
			{
				my $text = substr($line, 12);

				if ($field eq 'accession')
				{
					my @acc = split(m/\s/, $text);
					
					if (scalar @acc >= 1)
					{
						if (not defined $id)
						{
							$id = $acc[0];
							$m->IndexValue('id', $id);
						}
						
						foreach my $acc (@acc)
						{
							$m->IndexText('accession', $acc);
						}
					}
					else
					{
						warn "no accessions?\n" unless ();
					}
				}
				elsif ($field eq 'locus' and $text =~ m/^(\S+)(\s+\d+ (aa|bp)?.+)/o)
				{
					$m->IndexText('locus', $2);
					$prot = $3 eq 'aa';
				}
				elsif ($field eq 'version' and $text =~ m/GI:(\d+)/o)
				{
					$m->IndexWord('gi', $1);
				}
				elsif ($field eq 'definition')
				{
					$title .= ' ' if defined $title;
					$title .= $text;
					$m->IndexText($field, $text);
				}
				elsif ($field eq 'project' and $text =~ /GenomeProject:(\d+)/o)
                {
                       $m->IndexWord('project', $1);
                }
				else
				{
					$m->IndexText($field, $text) if defined $text;
				}
			}
		}
	}
}

#formatting

my @links = (
	{
		match	=> qr|^(DBSOURCE\s+REFSEQ:\s+accession\s+)([A-Z0-9_]+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=refseq&id=$2"}, $2)'
	},
	{
		match	=> qr|(/db_xref="embl:)(\S+)(?=[;"])|i,
		result	=> '$1.$q->a({-href=>"$url?db=embl&query=acc:$2"}, $2)'
	},
	{
		match	=> qr[(/db_xref="(swiss-prot|sptrembl|uniprot/swiss-prot|uniprot/trembl):)(\S+)(?=[;"])]i,
		result	=> '$1.$q->a({-href=>"$url?db=uniprot&query=ac:$3"}, $3)'
	},
	{
		match	=> qr|(/db_xref="taxon:)(\S+)(?=[;"])|i,
		result	=> '$1.$q->a({-href=>"$url?db=taxonomy&id=$2"}, $2)'
	},
);

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;
	
	$text = $this->link_url($text);
	
	foreach my $l (@links)
	{
		$text =~ s/$l->{match}/$l->{result}/eegm;
	}
	
	return $q->pre($text);
}

1;
