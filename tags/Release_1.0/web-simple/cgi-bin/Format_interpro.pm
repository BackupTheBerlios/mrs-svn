#!/opt/local/bin/perl -w
#
# $Id: Format_interpro.pm,v 1.4 2005/11/08 15:19:19 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;
use XML::XSLT;

package Format_interpro;

use Data::Dumper;
use XML::XSLT;

our @ISA = "Format";

my @links = (
	{
		match	=> qr|^(#=GF DR\s+PFAMA;\s)(\S+)(?=;)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfama&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PFAMB;\s)(\S+)(?=;)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pfamb&query=ac:$2"}, $2)'
	},
	{
		match	=> qr|^(#=GF DR\s+PDB;\s)(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=pdb&id=$2"}, $2)'
	},
	{
		match	=> qr|^(#=GS .+AC )(\S+)|i,
		result	=> '$1.$q->a({-href=>"$url?db=uniprot&query=ac:$2"}, $2)'
	},
);


sub new
{
	my $invocant = shift;
	my $self = {
		name => 'interpro',
		@_
	};
	my $result = bless $self, "Format_interpro";
	return $result;
}

my $stylesheet=<<'END';
<?xml version="1.0"?>

<xsl:stylesheet>
	<xsl:template match="dbinfo">
		<tr>
			<td><xsl:value-of select="@dbname"/></td>
			<td><xsl:value-of select="@version"/></td>
			<td><xsl:value-of select="@entry_count"/></td>
			<td><xsl:value-of select="@file_date"/></td>
		</tr>
	</xsl:template>

	<xsl:template match="release">
		<table>
			<xsl:apply-templates/>
		</table>
	</xsl:template>

</xsl:stylesheet>
END
#'

sub pp
{
	my ($this, $q, $text) = @_;
	
	my $parser = XML::XSLT->new(Source => "interpro_xslt.xml");
	
	my $xml = $parser->serve($text, xml_declaration=>0, http_headers=>0);
	
	$xml =~ s|#PUBMED#(\d+)|<a href="http://www.ncbi.nlm.nih.gov/entrez/utils/qmap.cgi?uid=$1&amp;form=6&amp;db=m&amp;Dopt=r">$1</a>|g;
	$xml =~ s|#INTERPRO#(IPR\d+)|<a href="mrs.cgi?db=interpro&amp;id=$1">$1</a>|g;
	$xml =~ s|#PDB#(.{4})|<a href="mrs.cgi?db=pdb&amp;id=$1">$1</a>|g;

	return $xml;
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my $desc;

	foreach my $line (split(m/\n/, $text))
	{
		if ($line =~ m|<name>(.+)</name>|)
		{
			$desc = $1;
			last;
		}
	}
	
	return $desc;
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'id' => 'Identification',
		'ac' => 'Accession number',
		'cc' => 'Comments and Notes',
		'dt' => 'Date',
		'de' => 'Description',
		'gn' => 'Gene name',
		'os' => 'Organism species',
		'og' => 'Organelle',
		'oc' => 'Organism classification',
		'ox' => 'Taxonomy cross-reference',
		'ref' => 'Any reference field',
		'dr' => 'Database cross-reference',
		'kw' => 'Keywords',
		'ft' => 'Feature table data',
		'sv' => 'Sequence version',
		'fh' => 'Feature table header'
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
