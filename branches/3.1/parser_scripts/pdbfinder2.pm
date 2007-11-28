# Perl module voor het parsen van pdb
#
# $Id: pdbfinder2.pm 79 2006-05-30 08:36:36Z maarten $
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

package MRS::Script::pdbfinder2;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'PDBFinder2',
		url			=> 'http://bioinformatics.oxfordjournals.org/cgi/content/abstract/12/6/525',
		section		=> 'structure',
		meta		=> [ 'title' ],
		raw_files	=> qr/^PDBFIND2\.TXT\.gz$/,
		@_
	};
	return bless $self, "MRS::Script::pdbfinder2";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $m, $state);

	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	$state = 0;

	# skip over comment header
	$lookahead = <IN> while (substr($lookahead, 0, 2) eq '//');
	
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;
		
		$doc .= $line;
		chomp($line);
		
		if (substr($line, 0, 2) eq '//')
		{
			if (defined $doc)
			{
				$m->Store($doc);
				$m->FlushDocument;
				
				$doc = undef;
			}
			
			$state = 0;
		}
		elsif ($state == 0)
		{
			if ($line =~ /^ID\s+:\s*(\S+)/o)
			{
				$m->IndexValue('id', $1);
			}
			elsif ($line =~ /^Chain/)
			{
				$state = 1;
			}
			elsif ($line =~ /^\s*(.+?)\s*:\s*(.*)/)
			{
				my ($key, $value) = (lc $1, $2);
				
				if ($key eq 'date')
				{
					$m->IndexDate('date', $value);
				}
				elsif ($key eq 't-nres-nucl' or
					$key eq 't-nres-prot' or
					$key eq 't-water-mols' or 
					$key eq 'het-groups')
				{
					$m->IndexNumber($key, $value);
				}
				elsif ($key eq 'resolution' or
					$key eq 'r-factor' or
					$key eq 'free-r')
				{
					$m->IndexNumber($key, $value * 1000.0);
				}
				elsif ($key eq 'header')
				{
					$m->StoreMetaData('title', lc $value);
					$m->IndexText($key, $value);
				}
				else
				{
					$value =~ s/(\w)\.(?=\w)/$1. /og
						if ($key eq 'author');

					if ($key =~ m/^[-a-z0-9]+$/io) {
						$m->IndexText($key, $value);
					}
					else {
						print "WARNING: invalid key '$key'\n";
					}
				}
			}
		}
	}

	if (defined $doc)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub sequence
{
	my ($self, $text) = @_;

	my $sequence;
	
	open TEXT, "<", \$text;
	while (my $line = <TEXT>) {
		if ($line =~ m/Sequence\s+:\s+(.+)/) {
			$sequence = $1;
			last;
		}
	}
	close TEXT;
	
	return $sequence;
}

1;
