# MRS plugin for creating an EMBL db
#
# $Id: uniseq.pm 169 2006-11-10 08:02:05Z hekkel $
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

package MRS::Script::uniseq;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'UniSeq',
		url			=> 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?db=unigene',
		section		=> 'gene',
#		meta		=> [ 'title' ],
		@_
	};
	return bless $self, "MRS::Script::uniseq";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($id, $doc, $state, $m);
	
	$state = 0;
	$m = $self->{mrs};
	
	my $lookahead = <IN>;
	while (defined $lookahead)
	{
		my $line = $lookahead;
		$lookahead = <IN>;

		if ($line =~ /^# Set/)
		{
			if (defined $doc and defined $id)
			{
				$m->Store($doc);
				$m->FlushDocument;
			}

			$id = undef;
			$doc = $line;
		}
		elsif (substr($line, 0, 1) eq '>')
		{
			$doc .= $line;
			chomp($line);

			$m->IndexText('text', $line);
			
			if (not defined $id and $line =~ m|/ug=(\S+)|)
			{
				$id = $1;
				$m->IndexValue('id', $id);
			}
		}
		else
		{
			$doc .= $line;
		}
	}
	
	if (defined $doc and defined $id)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}
}

sub raw_files
{
	my ($self, $raw_dir) = @_;

	my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";

	$raw_dir =~ s|[^/]+$||;
	$raw_dir .= "unigene";

	my @result;

	opendir DIR, $raw_dir;
	while (my $d = readdir DIR)
	{
		next unless -d "$raw_dir/$d";
		next if substr($d, 0, 1) eq '_';

		opendir D2, "$raw_dir/$d";
		my @files = grep { -e "$raw_dir/$d/$_" and $_ =~ /\.seq\.all\.gz$/ } readdir D2;
		push @result, join(' ', map { "$raw_dir/$d/$_" } @files) if scalar @files;
		closedir D2;
	}
	closedir DIR;

	return map { "gunzip -c $_ |" } @result;
}

1;
