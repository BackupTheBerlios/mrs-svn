# Perl module voor het parsen van pmc (PubMed Central)
#
# $Id: pmc.pm 18 2006-03-01 15:31:09Z hekkel $
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

package MRS::Script::pmc;

use XML::XSLT;

our @ISA = "MRS::Script";

my %INDICES = (
	'id'			=> 'PubMed ID',
	'doi'			=> 'DOI',
	'subject'		=> 'Subject',
	'title'			=> 'Title',
	'authors'		=> 'Authors',
	'aff'			=> 'Affiliated with',
	'abstract'		=> 'Abstract',
	'body'			=> 'Body text',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'PubMed Central',
		url			=> 'http://www.pubmedcentral.nih.gov/',
		section		=> 'literature',
		meta		=> [ 'title', 'doi' ],
		raw_files	=> qr/articles\.tar\.gz$/,
		ext_mapping	=> {
			extension	=> qr/.+\.tar\.gz$/,
			command		=> '"tar xzOf $file|"'
		},
		indices		=> \%INDICES,
		inited		=> 0,
		@_
	};
	return bless $self, "MRS::Script::pmc";
}

sub add_doc
{
	my ($m, $doc) = @_;
	
	local($/) = undef;
	
	$doc =~ s/^\s+//;

	if (length($doc) > 0) {
		eval {
			$m->AddXMLDocument($doc);
		};
		
		if ($@) {
			print "Adding XML document failed with '$@': ", $MRS::errstr, "\n\n";
			print $doc, "\n\n";
		}
	}
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	$| = 1;
	
	my $m = $self->{mrs};
	
	if ($self->{inited} == 0) {
		
		$m->AddXPathForIndex('pmid',		0, 1, 0, '//article/front/article-meta/article-id[@pub-id-type=\'pmid\']');
		$m->AddXPathForIndex('doi',			0, 1, 1, '//article/front/article-meta/article-id[@pub-id-type=\'doi\']');
		$m->AddXPathForIndex('subject',		0, 1, 0, '//article/front/article-meta/article-categories/subj-group[@subj-group-type=\'heading\']/subject');
		$m->AddXPathForIndex('title',		0, 1, 1, '//article/front/article-meta/title-group/article-title');
		$m->AddXPathForIndex('authors',		0, 1, 0, '//article/front/article-meta/contrib-group/contrib/name/*');
		$m->AddXPathForIndex('aff',			0, 1, 0, '//article/front/article-meta/aff');
		$m->AddXPathForIndex('abstract',	0, 1, 0, '//article/front/article-meta/abstract');
		$m->AddXPathForIndex('body',		0, 1, 0, '//article/body');
				
		$self->{inited} = 1;
	}
	
	my ($doc, $line, $xml_header, $n);
	
	# we're reading all documents concatenated as one large doc. That means we have to be careful to split
	# the stream at the right locations
	
	while ($line = <IN>)
	{
		my @lines = split(m/<!DOCTYPE.+?>|<\?xml.+?>/, $line);

		if (scalar(@lines) > 2) {
			print "$line\n";
		}
		elsif (scalar(@lines) == 2) {
			$doc .= $lines[0];
			&add_doc($m, $doc);
			$doc = $lines[1];
		}
		else {
			$doc .= $line;
		}	
	}

	if (defined $doc) {
#		$m->AddXMLDocument($doc);
		&add_doc($m, $doc);
	}
}

sub pp
{
	my ($this, $q, $text, $id, $url) = @_;

	my $result;

	eval
	{
		open X, ">/tmp/input-$$.xml";
		print X $text;
		close X;

#		our $mrs_bin_dir;
#		
#		my $settings = 'mrs.conf';
#		unless (my $return = do $settings)
#		{
#			warn "couldn't parse $settings: $@" if $@;
#			warn "couldn't do $settings: $!" unless defined $return;
#			warn "couldn't run $settings" unless $return;
#		}
#
#		$mrs_bin_dir = "/opt/local/bin/" unless defined $mrs_bin_dir;
		my $mrs_script_dir = $this->script_dir();
	
		open X, "Xalan /tmp/input-$$.xml $mrs_script_dir/viewnlm-v2.xsl|" or die "Could not run Xalan";
		local($/) = undef;
		$result = <X>;
		close X;
	};
	
	if ($@)
	{
		$result = $q->pre($@);
	}
	unlink "/tmp/input-$$.xml";
	
	return $result;
}

1;
