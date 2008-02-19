package MRS::Script::rebase;

use strict;

our @ISA = "MRS::Script";

my %INDICES = (
	'id' => 'Identification',
	'ac' => 'Accession number',
	'com' => 'Commercial provider code (see REBASE commercial data)',
	'et' => 'Enzyme type',
	'os' => 'Organism species',
	'pt' => 'Prototype enzyme',
	'ref' => 'Any reference field',
	'target' => 'Target',
	'targetlen' => 'Target length',
);

sub new
{
	my $invocant = shift;
	my $self = {
		name		=> 'REBASE',
		url			=> 'http://rebase.neb.com/rebase/rebase.html',
		section		=> 'other',
		meta		=> [ 'title' ],
		raw_files	=> qr/bairoch\./,
		indices		=> \%INDICES,
		@_
	};
	return bless $self, "MRS::Script::rebase";
}

sub parse
{
	my $self = shift;
	local *IN = shift;
	
	my ($doc, $state, $m, $osvalue);
	
	$state = 0;
	$m = $self->{mrs};
	
	
	while (my $line = <IN>) {
          if ($state == 0) {  # parsing header
	    if (substr($line, 0, 2) ne 'CC') {
	      $state = 1;
	    }
       	   } else { # parsing entries
		$doc .= $line;
		chomp($line);

		if (substr($line, 0, 2) eq '//') {
			$m->Store($doc);
			$m->FlushDocument;

			$doc = undef;
		}
		elsif ($line =~ /^([A-Z]{2}) {3}/o)
		{
			my $fld = $1;
			my $value = substr($line, 5);

			if ($fld eq 'ID') {
				$m->IndexValue('id', $value);
			}
                        elsif ($fld eq 'OS') {  # organism field
				$m->IndexTextAndNumbers('os', $value);
				$osvalue = $value;
			}
			elsif ($fld eq 'RS') {  # target line
			  $m->StoreMetaData('title', "$osvalue target:$value");
				# since no DE line, use this as title
			  $value =~ /([^,]+),.+;(([^,]+),.+;)?/;
			  $m->IndexWord('target', lc($1));
			  if (not $1 eq '?') { $m->IndexNumber('targetlen', length $1) } 
			  if ($2) {
			    $m->IndexWord('target', lc($3));
			    $m->IndexNumber('targetlen', length $3);
                          }
			}
			elsif (substr($fld, 0, 1) eq 'C') { # commercial
			  for my $letter ('A' .. 'Z') {
			    if ($value =~ /$letter/) { $m->IndexWord('com', lc $letter) }
			  }
			}
			elsif (substr($fld, 0, 1) eq 'R') { # reference, RA or RL
				$m->IndexTextAndNumbers('ref', $value);
			}
			elsif ($fld eq 'MS') {  # useless fields
			}
			else
			{
				$m->IndexText(lc($fld), substr($line, 5));
			}
               }
	  }
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
	my ($this, $q, $text, $id, $url) = @_;
	
	my ($result, $id);
	
	foreach my $line (split(m/\n/, $text))
	{
		if (substr($line, 0, 2) eq 'ID')
		{
			$id = substr($line, 5);
		}
		if (substr($line, 0, 2) eq 'PT')
		{
			if (substr($line, 5) ne $id) {
			$line =~ s/PT   (.+)$/PT   <a href='$url?db=rebase\&id=$1'>$1<\/a>/;
			}
		}
		if (substr($line, 0, 1) eq 'C')
		{
			if ($line =~ /C[RM]   ([A-Z]+)\./) {
				 my $comcodes = $1;
				 $comcodes =~ s/[A-Z]/$&\|/g;
				 chop $comcodes;
				 $line =~ s/(C[RM])   ([A-Z]+)/$1   <a href='query.do?db=rebase_commdata\&query=id:$comcodes'>$2<\/a>/;
			}
		}
		
		$result .= "$line\n";
	}
	
	return $q->pre($result);
}

1;

