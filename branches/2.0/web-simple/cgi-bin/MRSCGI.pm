# Common routines for the MRS interface

use strict;
use CGI;

package MRSCGI;

use strict;

our @ISA = "CGI";

sub new
{
	my $invocant = shift;
	
	my $self = { @_ };
	my $result = bless $self, "MRSCGI";

	$self->{script} = $0 unless defined $self->{script};
	$self->{script} =~ s|^.*/([^/]+)$|$1|;

	$self->init();

	my $base_url = $self->url({-absolute=>1});
	$base_url =~ s|/cgi-bin/$self->{script}$||;
	$base_url =~ s|/$||;
	
	$self->base_url($base_url);
	
	return $result;
}

sub menu($)
{
	my ($self, @buttons) = @_;

	my @links;
	my $base_url = $self->{base_url};

	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs.cgi?overview=1"}, "Overview of indexed databanks"));
	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs.cgi"}, "Search"));
#	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs.cgi?extended=1"}, "Extended Search"));
#	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/crc64.cgi"}, "Find Protein"));
#	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/blast.cgi"}, "NCBI-Blast"));
	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs-blast.cgi"}, "Blast"));
	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/result.cgi"}, "Blast results"));
	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs-clustalw.cgi"}, "ClustalW"));
	push @links, $self->li($self->a({-href=>"$base_url/cgi-bin/mrs.cgi?help=0"}, "Help"));
	push @links, $self->li($self->a({-href=>"$base_url/download/index.html"}, "Download"));

	return $self->div({-id=>'side'}, $self->ul(@links, @buttons));
}

sub lijst
{
	my ($self, @p) = @_;
	
	my $result = '';
	
	$result = $self->div({-class=>'list'},
		$self->table({-cellspacing=>'0', -cellpadding=>'0'}, @p)
	) if scalar @p;
	
	return $result;
}

sub base_url
{
	my ($self, $value) = @_;
	$self->{base_url} = $value if $value;
	return $self->{base_url};
}

sub browserIsIE
{
	my ($self) = @_;
	my $ua = $self->user_agent;
	return 1;#$ua =~ /MSIE\s+(\d.\d)/;
}

1;
