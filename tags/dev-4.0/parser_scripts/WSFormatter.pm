#!perl

package MRS::Script;

#use strict;
#use warnings;

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'default',
		@_
	};
	my $result = bless $self, "MRS::Script";
	return $result;
}

sub make_url
{
	my $url = shift;
	my $trailing_dot = '';

	if ($url =~ m/\.$/) {
		$url =~ s/\.$//;
		$trailing_dot = '.';
	}

	return "<a href=\"$url\">$url</a>$trailing_dot";
}

sub link_url
{
	my ($this, $text) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(([-\$_.A-Za-z0-9!*'(),/?=:&;[\]\|+]+\|%[0-9A-Fa-f]{2})+)|&make_url($&)|ge;
	
	return $text;
}

sub pp
{
	my ($this, $q, $text, $id) = @_;
	
	$text = $this->link_url($text);
	
	return $q->pre($text);
}

sub describe
{
	my ($this, $q, $text) = @_;
	
	my @lines = split(m/\n/, $text);
	splice(@lines, 7);
	push @lines, "...";
	$text = join("\n", @lines);
	
	return $q->pre($text);
}

sub script_dir
{
	my ($this, $script_dir) = @_;
	$this->{script_dir} = $script_dir if defined $script_dir;
	return $this->{script_dir};
}

sub index_name
{
	my ($self, $index) = @_;
	
	my $result = $self->{indices}->{$index};
	$result = $index unless defined $result;

	return $result;
}

package MRS::Script::ParserInterface;

use strict;
use warnings;
use XML::LibXML;
use XML::LibXML::XPathContext;

# code needed for Find Similar

sub new
{
	my $invocant = shift;

	my %meta;

	my $self = {
		name => 'default',
		text => [],
		meta => \%meta,
		@_
	};
	my $result = bless $self, "MRS::Script::ParserInterface";
	return $result;	
}

sub AddXPathForIndex
{
	my ($self, $index, $isValueIndex, $indexNumbers, $storeAsMetaData, $xPath) = @_;
	
	my $xpaths = $self->{xpaths};
	if (not defined $xpaths) {
		my %xpaths = (
			'$index' =>	[ $xPath ]
		);
		$self->{xpaths} = \%xpaths;
	}
	else {
		push @{$xpaths->{$index}}, $xPath;
	}
}

sub AddXMLDocument
{
	my ($self, $doc) = @_;
	
	my $parser = XML::LibXML->new;
	my $xdoc = $parser->parse_string($doc);
	my $xpc = XML::LibXML::XPathContext->new($xdoc);
	
	foreach my $ix (keys %{$self->{xpath}}) {
		foreach my $xpath (@{$ix}) {
			my @nodes = $xpc->find($xpath);
			
			foreach my $node (@nodes) {
				print "textContent: ", $node->textContent, "\n"; 
			}
		}
	}
}

sub SplitWords
{
	my ($self, $text) = @_;
	
	my @result;
	$text =~ s/([0-9a-z_]+((-|\.)[0-9a-z_]+)*)/push @result, $1;/ieegm;
	return map { lc $_ } @result;
}

sub IndexText
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, $self->SplitWords($text);
}

sub IndexTextAndNumbers
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, $self->SplitWords($text);
}

sub IndexWord
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, lc $text;
}

sub IndexValue
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, lc $text;
}

sub IndexWordWithWeight
{
	my ($self, $index, $text, $freq) = @_;
	
	while ($freq-- > 0)
	{
		push @{$self->{words}}, lc $text;
	}
}

sub IndexDate
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, lc $text;
}

sub IndexNumber
{
	my ($self, $index, $text) = @_;
	
	push @{$self->{words}}, lc $text;
}

sub AddSequence {}
sub Store {}

sub StoreMetaData
{
	my ($self, $name, $value) = @_;
	
	$self->{meta}->{$name} = $value;
}

sub FlushDocument {}

package Embed::WSFormat;

#use strict;
#use warnings;
use CGI;
use Data::Dumper;

my $q = new CGI;
my %formatters;

sub getScript
{
	my ($mrs_format_dir, $name) = @_;

	$name = 'default' if ((not defined $name) or (length($name) == 0));

	if ($name eq 'default')
	{
		$formatters{$name} = MRS::Script->new(script_dir => $mrs_format_dir) unless defined $formatters{$name};
	}
	else
	{
		my $plugin = "${mrs_format_dir}/${name}.pm";
		my $mtime = -M $plugin;
		
		if (not defined $formatters{$name} or $formatters{$name}{mtime} != $mtime )
		{
	        local *FH;
	        open FH, $plugin or die "open '$plugin' $!";
	        local($/) = undef;
	        my $sub = <FH>;
	        close FH;
	
            eval $sub;
	        die $@ if $@;
	
	        #cache it unless we're cleaning out each time
			$formatters{$name} = "MRS::Script::$name"->new(script_dir => $mrs_format_dir);
	        $formatters{$name}{mtime} = $mtime;
		}
	}
	
	return $formatters{$name};
};

sub html
{
	my ($mrs_format_dir, $name, $text, $db, $id) = @_;
	
	my $result;
	
	eval
	{
		my $url = undef;

		my $fmt = &getScript($mrs_format_dir, $name);

		$result = $fmt->pp($q, $text, $id, $url);
	};
	
	if ($@)
	{
		$result = "Error in formatting entry: $@";
	}
	
	return $result;
}

sub title
{
	my ($mrs_format_dir, $name, $text, $db, $id) = @_;
	
	my $result;

	$name = $db unless defined $name and length($name) > 0;

	eval
	{
		my $url = undef;

		my $m = new MRS::Script::ParserInterface;
		my $p = &getScript($mrs_format_dir, $name);
		$p->{mrs} = $m;

		open TEXT, '<', \$text;
		$p->parse(*TEXT, 0, $db, undef);
		close TEXT;

		$result = $m->{meta}->{title};

		if (not defined $result) {
			$result = $p->describe($q, $text, $id, $url);
		}
	};
	
	if ($@)
	{
		$result = "Error in formatting title: $@";
	}
	
	return $result;
}

sub indexed
{
	my ($mrs_format_dir, $name, $text, $db, $id) = @_;
	
	my $result;

	eval
	{
		my $url = undef;

		my $m = new MRS::Script::ParserInterface;

		my $p = &getScript($mrs_format_dir, $name);
		$p->{mrs} = $m;
		
		open TEXT, '<', \$text;
		$p->parse(*TEXT, 0, $db, undef);
		close TEXT;

		$result = join(" ", @{$m->{words}});
	};
	
	if ($@)
	{
		$result = "Error in indexing entry: $@";
	}
	
	return $result;
}

sub index_name
{
	my ($mrs_format_dir, $name, $index) = @_;
	
	my $result;

	eval
	{
		my $url = undef;

		my $p = &getScript($mrs_format_dir, $name);
		
		$result = $p->index_name($index);
	};
	
	if ($@)
	{
		$result = "Error in retrieving index name: $@";
	}
	
	return $result;
}

1;
