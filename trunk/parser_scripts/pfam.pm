# MRS plugin for creating an pfam db
#
# $Id: pfam.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::pfam;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	
	my $self = {
		url		=> 'http://www.sanger.ac.uk/Software/Pfam/',
		section	=> 'other',
		meta	=> [ 'title' ],
		@_
	};
	
	if (defined $self->{db}) {
		
		my %NAMES = (
			pfama		=> 'Pfam-A',
			pfamb		=> 'Pfam-B',
			pfamseed	=> 'Pfam-Seed'
		);

		$self->{name} = $NAMES{$self->{db}};
		
		my %FILES = (
			pfama		=> qr/Pfam-A\.full\.gz/,
			pfamb		=> qr/Pfam-A\.seed\.gz/,
			pfamseed	=> qr/Pfam-B\.gz/,
		);
		
		$self->{raw_dir} =~ s|pfam[^/]+/?$|pfam|;
		$self->{raw_files} = $FILES{$self->{db}};
	}
	
	return bless $self, "MRS::Script::pfam";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $state, $m);
	
	$state = 0;
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
			$m->Store($doc);
			$m->FlushDocument;

			$doc = undef;
		}
		elsif (substr($line, 0, 5) eq '#=GF ')
		{
			my $field = substr($line, 5, 2);
			my $value = substr($line, 10);

			if ($field eq 'ID')
			{
				$m->IndexValue('id', $value);
			}
			elsif ($field eq 'AC' and $value =~ m/(P[BF]\d+)/)
			{
				$m->IndexValue('ac', $1);
			}
			elsif (substr($field, 0, 1) eq 'R')
			{
				$m->IndexText('ref', substr($line, 5));
			}
			else
			{
				$m->StoreMetaData('title', $value) if $field eq 'DE';
				$m->IndexText(lc($field), substr($line, 5));
			}
		}
		elsif (substr($line, 0, 5) eq '#=GS ')
		{
			$m->IndexText('gs', substr($line, 5));
		}
	}
}

my @links = (
	{
		match	=> qr|^(#=GF DR\s+PFAMA;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=pfama&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PFAMB;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=pfamb&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PDB;\s)(\S+)|,
		result	=> '$1.$q->a({-href=>"$url?db=pdb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(#=GS .+AC )(\S+)|,
		result	=> '$1.$q->a({-href=>"$url?db=uniprot&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PROSITE;\s)(\S+)(?=;)|,
		result	=> '$1.$q->a({-href=>"$url?db=prosite_doc&id=$2"}, $2)'
	},
);

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $url = $q->url({-full=>1});
	
	$text = $this->link_url($text);
	
	foreach my $l (@links)
	{
		$text =~ s/$l->{match}/$l->{result}/eegm;
	}
	
	return $q->pre($text);
}

1;
