# 2007-02-21 written by Guy Bottu for BEN

package MRS::Script::rebase_commdata;

use strict;

our @ISA = "MRS::Script";

my %INDICES = (
	'id' => 'Identification',
	'com' => 'Commercial provider name',
	'enz' => 'Enzyme name',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'REBASE commercial data',
		url			=> 'http://rebase.neb.com/rebase/rebase.html',
		section		=> 'other',
		meta		=> [ 'title' ],
		raw_files	=> qr/commdata\./,
		indices		=> \%INDICES,
		@_
	};

	$self->{raw_dir} =~ s|_commdata/?$|| if defined $self->{raw_dir};

	return bless $self, "MRS::Script::rebase_commdata";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $data, $m);
	
	$m = $self->{mrs};
	
	
	# for rebase_commdata it is most convenient to read the complete db into a file
	# and then split it in records
	while (<IN>) {
		$data .= $_;
	}

	my @entry = split /-+\n/, $data;
	shift @entry;  # drop header databank

	for my $doc (@entry) {
		$m->Store($doc);
		my @block = split /\n\n+/, $doc;
		my $state = 0;
		for my $block (@block) {
			if ($state == 0) { # parsing company information
				if ($block =~ /^\n?(.+)   \(REBASE abbr: ([A-Z])\)/) {
					$m->IndexText('com', $1);
					$m->StoreMetaData('title', $1);
					$m->IndexValue('id', "$2");
				} elsif ($block =~ /^As of:/) {
					$state = 1;
				}
			}
			else { # parsing enzymes information
				if ($block =~ /^Distributors for/) {
					last; # info about distributors
				} elsif ($state == 1) {
					$state = 2;
				} elsif ($state == 2) {
					my @enzyme = split /[ \n]+/, $block;
					for my $enz (@enzyme) {
						$m->IndexWord('enz', lc $enz);
						$state = 1;
					}
				}
			}
		}
		$m->FlushDocument;
	}
}

sub version
{
	my ($self) = @_;

	my $raw_dir = $self->{raw_dir} or die "raw_dir is not defined\n";

	my $vers = `cat $raw_dir/VERSION`;
	chomp $vers;

	return $vers;
}

# formatting

sub pp
{
	my ($this, $q, $text) = @_;

	return $q->pre($text);
}

1;
