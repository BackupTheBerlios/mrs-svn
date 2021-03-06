# MRS plugin for creating a db
#
# $Id$
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

package genbank::parser;

use strict;

my $count = 0;

our $COMPRESSION_LEVEL = 7;
our $COMPRESSION = "zlib";

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "genbank::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $id, $m, $done, $field, $prot);
	
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
			$m->Store($doc);
			$m->FlushDocument;

			$doc = undef;
			$field = undef;
			$id = undef;
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
					my @acc = split(/\s/, $text);
					
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
				elsif ($field eq 'locus' and $text =~ /^(\S+)(\s+\d+ (aa|bp)?.+)/o)
				{
#					$m->IndexValue('id', $1);
					$m->IndexText('locus', $2);
					$prot = $3 eq 'aa';
				}
				elsif ($field eq 'version' and $text =~ /GI:(\d+)/o)
				{
					$m->IndexWord('gi', $1);
				}
				else
				{
					$m->IndexText($field, $text) if defined $text;
				}
#				elsif ($field eq 'accession' or
#					   $field eq 'comment' or
#					   $field eq 'contig' or
#					   $field eq 'dbsource' or
#					   $field eq 'definition' or
#					   $field eq 'keywords' or
#					   $field eq 'organism' or
#					   $field eq 'source')
#				{
#					$m->IndexText($field, $text);
#				}
#				else
#				{
#					warn "skipping $field in $line\n"
#				}
			}
		}
	}
}

sub version
{
	my ($self, $raw_dir, $db) = @_;
	
	my $vers;
	
	if ($db eq 'genbank_release')
	{
		open RELDATE, "<$raw_dir/README.genbank";
		
		while (my $line = <RELDATE>)
		{
			if ($line =~ /GenBank Flat File Release/) {
				$vers = $line;
				last;
			}
		}
		
		close RELDATE;
	}

	die "Unknown db: $db" unless defined $vers;

	chomp($vers);

	return $vers;
}

sub raw_files
{
	my ($self, $raw_dir) = @_;

	opendir DIR, $raw_dir;
	my @result = grep { -e "$raw_dir/$_" and $_ =~ m/\.gz$/ } readdir DIR;
	closedir DIR;
	
	return map { "gunzip -c $raw_dir/$_ |" } @result;
}

1;
