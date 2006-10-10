#!/opt/local/bin/perl -w
#
# $Id: Format_gene.pm,v 1.4 2005/11/08 15:19:19 maarten Exp $
#
# Copyright 2005, M.L. Hekkelman. CMBI, Radboud Universiteit Nijmegen
#

use strict;

package Format_gene;

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
		result	=> '$1.$q->a({-href=>"$url?db=sprot%2Btrembl&query=ac:$2"}, $2)'
	},
);


sub new
{
	my $invocant = shift;
	my $self = {
		name => 'gene',
		@_
	};
	my $result = bless $self, "Format_gene";
	return $result;
}

sub pp
{
	my ($this, $q, $text) = @_;

	$text =~ s/<\!DOCTYPE.+?>\n(<Entrezgene-Set>)?/<Entrezgene-Set>/;

	my $result;

	eval
	{
		open X, ">/tmp/input-$$.xml";
		print X $text;
		close X;

		our $mrs_bin_dir;
		
		my $settings = 'mrs.conf';
		unless (my $return = do $settings)
		{
			warn "couldn't parse $settings: $@" if $@;
			warn "couldn't do $settings: $!" unless defined $return;
			warn "couldn't run $settings" unless $return;
		}
	
		open X, "$mrs_bin_dir/Xalan /tmp/input-$$.xml gene_xslt.xml|";
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

sub describe
{
	my ($this, $q, $text) = @_;

	$text =~ s/<\!DOCTYPE.+?>\n(<Entrezgene-Set>)?/<Entrezgene-Set>/;

	my $result;

	eval
	{
		open X, ">/tmp/input-$$.xml";
		print X $text;
		close X;

		our $mrs_bin_dir;
		
		my $settings = 'mrs.conf';
		unless (my $return = do $settings)
		{
			warn "couldn't parse $settings: $@" if $@;
			warn "couldn't do $settings: $!" unless defined $return;
			warn "couldn't run $settings" unless $return;
		}
	
		open X, "$mrs_bin_dir/Xalan /tmp/input-$$.xml gene_list_xslt.xml|";
		local($/) = undef;
		$result = <X>;
		close X;
	};
	
	if ($@)
	{
		$result = $q->pre($@);
	}
#	unlink "/tmp/input-$$.xml";
	
	return $result;
}

sub to_field_name
{
	my ($this, $id) = @_;
	
	my %n = (
		'id' => 'Identification',
	);

	my $result = $n{$id};
	$result = $id unless defined $result;
	return $result;
}

1;
