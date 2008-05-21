#!perl

use warnings;
use SOAP::Lite;
use Data::Dumper;
use Getopt::Std;

my %opts;
getopts('d:e:', \%opts);
my $db = $opts{'d'} or die "Please specify databank\n";
my $e = $opts{'e'} or die "Please specify id\n";

my $ns_url = 'http://mrs.cmbi.ru.nl/mrsws';
my $ns = 'service';
#my $url = 'http://mrs.cmbi.ru.nl/mrsws';
my $url = 'http://localhost:8081/mrsws';

my $soap = SOAP::Lite->uri($ns_url)->proxy($url);

print &getEntry($db, $e);
exit;

sub soapCall()
{
    my $result = $soap->call(@_);
    
    my $err;
    if ($result->fault)
    {
        $err .= $result->faultdetail."\n"	if defined $result->faultdetail;
        $err .= $result->faultcode."\n"		if defined $result->faultcode;
        $err .= $result->faultstring."\n"	if defined $result->faultstring;
        $err .= $result->faultactor."\n"	if defined $result->faultactor;
    }

    return ($result->result, $err) if defined wantarray and wantarray;
    die $err if defined $err;
    return $result->result;
};

sub getEntry
{
	my ($db, $id) = @_;

	my $r = &soapCall(SOAP::Data->name("$ns:GetEntry")->attr({"xmlns:$ns" => $ns_url})
	    => (
	        SOAP::Data->name("$ns:db")        ->type('xsd:string' => $db),
	        SOAP::Data->name("$ns:id")        ->type('xsd:string' => $id)
	    ));
	
	return $r;
}

