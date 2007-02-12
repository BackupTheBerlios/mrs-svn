# Perl module voor het parsen van pdb
#
# $Id: omim.pm 169 2006-11-10 08:02:05Z hekkel $
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

package MRS::Script::omim;

our @ISA = "MRS::Script";

my %INDICES = (
	'no' => 'Number',
	'id' => 'Number',
	'ti' => 'Title',
	'mn' => 'Mini-Mim',
	'av' => 'Allelic variation',
	'tx' => 'Text',
	'sa' => 'See also',
	'rf' => 'References',
	'cs' => 'Clinical Synopsis',
	'cn' => 'Contributor name',
	'cd' => 'Creation name',
	'cd_date' => 'Creation date',
	'ed' => 'Edit history',
	'ed_date' => 'Edit history (date)',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'OMIM - Online Mendelian Inheritance in Man™',
		url			=> 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=OMIM',
		section		=> 'literature',
		meta		=> [ 'title' ],
		raw_files	=> qr/omim\.txt\.Z/,
		indices		=> \%INDICES,
		@_
	};
	return bless $self, "MRS::Script::omim";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $field, $title, $m, $state);

	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		if ($line =~ /^\*RECORD/o)
		{
			if (defined $doc)
			{
				$m->StoreMetaData('title', lc $title) if defined $title;
				$m->Store($doc);
				$m->FlushDocument;
			}
			
			$doc = $line;
			
			$field = undef;
			$title = undef;
			$state = 1;
		}
		else
		{
			$doc .= $line;
			chomp($line);
			
			if ($line =~ /^\*FIELD\* (..)/o)
			{
				$field = lc($1);
			}
			elsif ($field eq 'no')
			{
				$m->IndexValue('id', $line);
			}
			else
			{
				if (($field eq 'cd' or $field eq 'ed') and $line =~ m|(\d{1,2})/(\d{1,2})/(\d{4})|)
				{
					my $date = sprintf('%4.4d-%2.2d-%2.2d', $3, $1, $2);
					
					eval { $m->IndexDate("${field}_date", $date); };
					
					warn $@ if $@;
				}
				
				if ($field eq 'ti' and not defined $title)
				{
					$title = $line;
					$title =~ s/^\D?\d{6}\s//om;
					$title = (split(m/;/, $title))[0];
				}
				
				$m->IndexText($field, $line);
			}
		}
	}

	if (defined $doc)
	{
		$m->StoreMetaData('title', lc $title) if defined $title;
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub pp
{
	my ($this, $q, $text) = @_;
	
	my %labels = (
		AV => 'Allelic variation',
		CD => 'Creation date',
		CN => 'Contributor name',
		CS => 'Clinical Synopsis',
		ED => 'Edit history',
		MN => 'Mini-Mim',
		NO => 'Number',
		RF => 'References',
		SA => 'See Also',
		TI => 'Title',
		TX => 'Text',
	);

	$text = $this->link_url($text);
	
	$text =~ s/^\*RECORD\*\s+//;
	
	my @fields = split(/^\*FIELD\* /om, $text);

	my $url = $q->url({-full=>1});
	
	my @data;
	
	foreach my $f (@fields)
	{
		my $code;
		
		if ($f =~ /^(\w\w)\s/)
		{
			$code = $1;
			my $label = $code;
			$label = $labels{$label} if defined $labels{$label};
			
			push @data, $q->h3($label);
			
			$f = substr($f, 3);
		}

		if ($code eq 'CS')
		{
			my @dl;
			foreach my $p (split(/\n\n/, $f))
			{
				my ($dt, $dd) = split(/:/, $p);
				push @dl, $q->dt($q->b("$dt:")), $q->dd($dd);
			}
			push @data, $q->dl(@dl);
			next;
		}

		if ($code eq 'CN' or $code eq 'ED')
		{
			$f = join($q->br, split(/\n/, $f));
			push @data, $q->p($f);
			next;
		}

		if ($code eq 'RF' and 0)
		{
			my @dl;
			foreach my $p (split(/\n\n/, $f))
			{
				my ($dt, $dd) = split(/:/, $p);
				push @dl, $q->dt("$dt:"), $q->dd($dd);
			}
			push @data, $q->dl(@dl);
			next;
		}
		
		foreach my $p (split(/\n\n/, $f))
		{
			if ($code eq 'TX' or $code eq 'AV')
			{
				$p =~ s|\((\S+?;\s+)?(\d{6})\)|($1<a class='bluelink' href='?db=omim\&id=$2'>$2</a>)|g;
				$p =~ s|\((\S+?;\s+)?(\d{6})\.(\d{4})\)|($1<a class='bluelink' href='?db=omim\&id=$2#.$3'>$2.$3</a>)|g;
				
				$p =~ s|^[A-Z ]+$|<h4>$&</h4>|mg;
			}
			
			if ($code eq 'AV')
			{
				$p =~ s|^\.(\d{4})|<a name='.$1'>.$1</a>|mg;
			}
			
			push @data, $q->p($p);
		}
	}
	
	return join("\n", @data);
}

1;
