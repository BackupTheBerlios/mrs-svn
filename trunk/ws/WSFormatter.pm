#!perl

package MRS::Script;

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

sub link_url
{
	my ($this, $text) = @_;
	
	$text =~ s/&/&amp;/g;
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	
	$text =~ s|http://(([-\$_.A-Za-z0-9!*'(),/?=:&;[\]\|+]+\|%[0-9A-Fa-f]{2})+)|<a href='$&'>$&</a>|g;
	
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

package MRS::Script::ParserInterface;

# code needed for Find Similar

sub new
{
	my $invocant = shift;
	my $self = {
		name => 'default',
		text => '',
		@_
	};
	my $result = bless $self, "MRS::Script::ParserInterface";
	return $result;	
}

sub IndexText
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub IndexTextAndNumbers
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub IndexWord
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub IndexValue
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub IndexWordWithWeight
{
	my ($self, $index, $text, $freq) = @_;
	
	while ($freq-- > 0)
	{
		$self->{text} .= "$text ";
	}
}

sub IndexDate
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub IndexNumber
{
	my ($self, $index, $text) = @_;
	
	$self->{text} .= "$text ";
}

sub AddSequence {}
sub Store {}
sub StoreMetaData {}
sub FlushDocument {}

package Embed::WSFormat;

use strict;
use CGI;

my $q = new CGI;
my %formatters;

sub getScript
{
	my ($mrs_format_dir, $name) = @_;

	if ($name eq 'default')
	{
		$formatters{$name} = MRS::Script->new(script_dir => $mrs_format_dir) unless defined $formatters{$name};
	}
	else
	{
		my $plugin = "${mrs_format_dir}${name}.pm";
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

	eval
	{
		my $url = undef;

		my $fmt = &getScript($mrs_format_dir, $name);

		$result = $fmt->describe($q, $text, $id, $url);
	};
	
	if ($@)
	{
		$result = "Error in formatting entry: $@";
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

		$result = $m->{text};
	};
	
	if ($@)
	{
		$result = "Error in indexing entry: $@";
	}
	
	return $result;
}

1;
