# MRS plugin for creating an prosite.dat db
#
# $Id: prosite.pm 179 2006-11-15 10:08:00Z hekkel $
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

package MRS::Script::prosite;

our @ISA = "MRS::Script";

my %INDICES = (
	'id' => 'Identification',
	'ac' => 'Accession number',
	'cc' => 'Comments and Notes',
	'de' => 'Description',
	'do' => 'PROSITE documentation link',
	'dr' => 'Database cross-reference',
	'dt' => 'Date',
	'pp' => 'Post-Processing Rule',
	'pr' => 'Prorule link',
	'ru' => 'Rule',
	'type' => 'Type (PATTERN, MATRIX or RULE)'
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Prosite',
		url			=> 'http://www.expasy.org/prosite/',
		section		=> 'function',
		meta		=> [ 'title' ],
		raw_files	=> qr/prosite\.dat/,
		indices		=> \%INDICES,
		@_
	};
	return bless $self, "MRS::Script::prosite";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	
	$lookahead = <IN>
		while (uc substr($lookahead, 0, 2) eq 'CC');
	$lookahead = <IN>
		if (substr($lookahead, 0, 2) eq '//');
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		$doc .= $line;

		chomp($line);

		if (substr($line, 0, 2) eq '//')
		{
			die "No ID specified\n" unless defined $id;

			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}/o)
		{
			my $fld = $1;
			my $value = substr($line, 5);

			if ($fld eq 'ID' and $value =~ /^([A-Z0-9_]+); ([A-Z]+)/o)
			{
				die "Double ID: $id <=> $1\n" if defined $id;
				$id = $1;
				die "ID too short" unless length($id);
				$m->IndexValue('id', $id);
				$m->IndexWord('type', lc $2);
			}
			elsif ($fld =~ /MA|NR|PA/o)  # useless fields
			{}
			else
			{
				$m->StoreMetaData('title', $value) if $fld eq 'DE';
				$m->IndexText(lc($fld), $value);
			}
		}
	}
}

sub version
{
	my ($self, $raw_dir) = @_;
	my $vers;

	open REL, "<$raw_dir/prosite.dat";

	while (my $line = <REL>)
	{
		if ($line =~ /^CC   (Release [0-9.]+ [^.]+)\./) {
			$vers = $1;
			last;	
		}
	}	

	close REL;

	return $vers;
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	my $result;
	
	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'DR')
		{
			$line =~ s{
						(\S+)(,\s+)(\S+)(?=\s*,\s+.;)
					}
					{
						"<a href='$url?db=sprot%2Btrembl\&query=ac:$1'>$1</a>" .
						"$2" .
						"<a href='$url?db=sprot%2Btrembl\&id=$3'>$3</a>"
					}xge;
		}
		elsif (substr($line, 0, 2) eq '3D') 
		{
			$line =~ s{
						(\w\w\w\w);
					}
					{
						"<a href='$url?db=pdb\&query=id:$1'>$1</a>;"
					}xge;
		}
		elsif (substr($line, 0, 2) eq 'DO')
		{
			$line =~ s{
						(PDOC\d+)
					}
					{
						$q->a({-href=>"$url?db=prosite_doc&id=$1"}, $1)
					}xge;
		}
		
		$result .= "$line\n";
	}
	
	return $q->pre($result);
}

1;
