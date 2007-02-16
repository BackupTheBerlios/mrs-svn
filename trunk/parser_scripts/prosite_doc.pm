# MRS plugin for creating an prosite.doc db
#
# $Id: prosite_doc.pm 179 2006-11-15 10:08:00Z hekkel $
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

package MRS::Script::prosite_doc;

our @ISA = "MRS::Script";

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'Prosite Documentation',
		url			=> 'http://www.expasy.org/prosite/',
		section		=> 'function',
		meta		=> [ 'title' ],
		raw_files	=> qr/prosite\.doc/,
		@_
	};
	
	$self->{raw_dir} =~ s|_doc/?$|| if defined $self->{raw_dir};
	
	return bless $self, "MRS::Script::prosite_doc";
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

		$doc .= $line;

		chomp($line);

		if ($line eq '{END}')
		{
			$m->Store($doc);
			$m->FlushDocument;

			$id = undef;
			$doc = undef;
		}
		elsif ($line =~ /^\{(PDOC\d+)\}/o)
		{
			$id = $1;
			$m->IndexValue('id', $id);
		}
		elsif ($line =~ m/\*\s*([^*].+?)\s*\*/) {
			$m->StoreMetaData('title', $1);
			$m->IndexText('text', $1);
		}
		else
		{
			$m->IndexText('text', $line);
		}
	}
}

sub version
{
    my ($self) = @_;
    my $vers;

	my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";
	$raw_dir =~ s/prosite_doc/prosite/;

    open REL, "<$raw_dir/prosite.doc";

    while (my $line = <REL>)
    {
        if ($line =~ /^(Release [0-9.]+ of [^.]+)\./) {
            $vers = $1;
            last;
        }
    }

    close REL;
    
    warn "Version not found" unless defined $vers;

    return $vers;
}

1;
