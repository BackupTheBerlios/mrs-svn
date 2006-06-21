# Perl module voor het parsen van pdb
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

package omim::parser;

my $count = 0;

sub new
{
	my $invocant = shift;
	my $self = {
		@_
	};
	return bless $self, "omim::parser";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $field, $m);

	$m = $self->{mrs};
	
	while (my $line = <IN>)
	{
		if ($line =~ /^\*RECORD/o)
		{
			if (defined $doc)
			{
				$m->Store($doc);
				$m->FlushDocument;
			}
			
			$doc = $line;
			
			$field = undef;
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
				
				$m->IndexText($field, $line);
			}
		}
	}

	if (defined $doc)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub raw_files()
{
	my ($self, $raw_dir) = @_;
	return "gunzip -c $raw_dir/omim.txt.Z|";
}

1;
