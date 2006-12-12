#!perl -w

use strict;
use MRS;
use Getopt::Std;

my %opts;
getopts('t:v', \%opts);

my $tmpDir = $opts{t};
$tmpDir = "/tmp/" unless defined $tmpDir;

$MRS::VERBOSE = 1 if defined $opts{v};

$| = 1;

&doTest001();
&doTest002();
exit;

sub printResults
{
	my $r = shift;
	
	while (my $id = $r->Next) {
		print "$id, ";
	}
	print "\n";
}

sub testResults
{
	my ($resultSet, @expected) = @_;
	
	print "expected : ", join(", ", @expected), "\n";
	
	my @r;
	my $ok = 1;
	
	while (my $id = $resultSet->Next)
	{
		my $rid = shift @expected;
		
		$ok = 0 unless $rid eq $id;
		
		push @r, $id;
	}
	
	print "found    : ", join(", ", @r), "\n";
	die "Test failed\n" unless $ok;
	
	print "Test passed\n";
}

sub doTest001()
{
	# test is a merged databank contains all required index entries
	
	my $a = MRS::MDatabank::Create("$tmpDir/a.cmp")
		or die "Could not create new databank a: " . MRS::errstr;
	my $b = MRS::MDatabank::Create("$tmpDir/b.cmp")
		or die "Could not create new databank b: " . MRS::errstr;
	
	$a->IndexText('een', 'aap');
	$a->IndexValue('id', 'a1');
	$a->Store('aap 1');
	$a->FlushDocument();
	
	$a->IndexText('een', 'noot');
	$a->IndexValue('id', 'a2');
	$a->Store('noot 1');
	$a->FlushDocument();
	
	$a->IndexText('een', 'mies');
	$a->IndexValue('id', 'a3');
	$a->Store('mies 1');
	$a->FlushDocument();
	
	$a->Finish();

	$b->IndexText('een', 'aap');
	$b->IndexValue('id', 'b1');
	$b->Store('aap 2');
	$b->FlushDocument();
	
	$b->IndexText('een', 'noot');
	$b->IndexValue('id', 'b2');
	$b->Store('noot 2');
	$b->FlushDocument();
	
	$b->IndexText('een', 'mies');
	$b->IndexValue('id', 'b3');
	$b->Store('mies 2');
	$b->FlushDocument();
	
	$b->Finish();

	$a = new MRS::MDatabank("$tmpDir/a.cmp");
	$b = new MRS::MDatabank("$tmpDir/b.cmp");
	
	my @dbs = ($a, $b);
	
	my $c = MRS::MDatabank::Merge("$tmpDir/c.cmp", \@dbs);
	
	$a = undef;
	$b = undef;
	$c = undef;
	
	$c = new MRS::MDatabank("$tmpDir/c.cmp");
	
	print "Created two databanks, a and b. Merged them to c. Now testing:\n";

	print "Search for key a1 in id index\n";
	&testResults($c->Find("id:a1", 0), "a1")
		or die "Could not do query: " . MRS::errstr;
	
	print "Search for key 'aap' in index 'een'\n";
	&testResults($c->Find("een:aap", 0), "a1", "b1")
		or die "Could not do query: " . MRS::errstr;

	print "Search for key 'noot' in index 'een'\n";
	&testResults($c->Find("een:noot", 0), "a2", "b2")
		or die "Could not do query: " . MRS::errstr;
	
	print "Search for key 'mies' in index 'een'\n";
	&testResults($c->Find("een:mies", 0), "a3", "b3")
		or die "Could not do query: " . MRS::errstr;

	
}

sub doTest002()
{
	# test if merging two db's fails on duplicate values
	
	my $a = MRS::MDatabank::Create("$tmpDir/a.cmp")
		or die "Could not create new databank a: " . MRS::errstr;
	my $b = MRS::MDatabank::Create("$tmpDir/b.cmp")
		or die "Could not create new databank b: " . MRS::errstr;
	
	$a->IndexText('een', 'aap');
	$a->IndexValue('id', 'a1');
	$a->Store('aap 1');
	$a->FlushDocument();
	
	$a->Finish();

	$b->IndexText('een', 'aap');
	$b->IndexValue('id', 'a1');
	$b->Store('aap 1');
	$b->FlushDocument();
	
	$b->Finish();

	$a = new MRS::MDatabank("$tmpDir/a.cmp");
	$b = new MRS::MDatabank("$tmpDir/b.cmp");
	
	my @dbs = ($a, $b);
	
	eval {
		print "Test if merging fails\n";
		my $c = MRS::MDatabank::Merge("$tmpDir/c.cmp", \@dbs);
		die "Merging should have failed\n";
	};
	
	if ($@) {
		print MRS::errstr, "\nTest passed\n";
	}
}

