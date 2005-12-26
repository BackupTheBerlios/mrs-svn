#!/usr/bin/perl -w

use strict;
use MRS;

my $count = 0;

$| = 1;
#$MRS::VERBOSE = 1;
&main();
exit;

sub parse
{
	my ($m, $f) = @_;
	
	my ($doc, $field);
	my $state = 0;
	
	open TEXT, $f;
	while (my $line = <TEXT>)
	{
		if ($line =~ /^From \S+(@| at )\S+\s+(Mon|Tue|Wed|Thu|Fri|Sat|Sun)/o)
		{
			if (defined $doc)
			{
				$m->Store($doc);
				$m->FlushDocument;
				print '.' if (++$count % 100) == 0;
			}

			$doc = $line;
			$field = undef;
			$state = 1;
		}
		elsif ($state == 1)
		{
			$doc .= $line;
			chomp($line);
			
			my $text;
			
			if (length($line) == 0)
			{
				$state = 2;
			}
			elsif ($line =~ /^(\S+):\s*(.+)/o)
			{
				$field = lc $1;
				$text = $2;
			}
			else
			{
				$text = $line;
			}
			
			if (defined $field and defined $text)
			{
				if ($field eq 'message-id')
				{
					if ($text =~ /^<(.+)>$/o)
					{
						$text = $1;
					}
					
					$m->IndexValue('id', $text);
				}
				elsif ($field eq 'references')
				{
					foreach my $r (split(m/\s+/, $text))
					{
						if ($r =~ /^<(.+)>$/o)
						{
							$r = $1;
						}

						$m->IndexWord('references', $r) if length($r) > 0;
					}
				}
				else
				{
					$m->IndexText($field, $text);
				}
			}
		}
		else
		{
			$doc .= $line;
		}
	}

	if (defined $doc)
	{
		$m->Store($doc);
		$m->FlushDocument;
	}

	close TEXT;
}

sub main
{
	my $d = '/Users/maarten/data/opensc.cmp';
	my $p = '/Users/maarten/projects/opensc-m/';
	
	my $m = MRS::Databank::Create($d)
		or die "Could not create opensc.cmp";
	
	opendir DIR, $p or die "Could not open dir";
	my @files = grep { m/.*\.txt\.gz$/ } readdir DIR;
	closedir DIR;
	
	foreach my $f (@files)
	{
		&parse($m, "gzcat $p/$f |");
	}

	print "\nCreating index... ";
	$m->Finish;
	print "\ndone\n";
	
	chmod 0644, $d;
}
