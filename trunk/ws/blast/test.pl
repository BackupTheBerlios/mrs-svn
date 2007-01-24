#!perl

use warnings;
use SOAP::Lite;
use Data::Dumper;

my $ns_url = 'http://mrs.cmbi.ru.nl/mrsws-blast';
my $ns = 'service';
my $url = 'http://localhost:8082/mrsws-blast';

my $soap = SOAP::Lite->uri($ns_url)->proxy($url);

sub soapCall()
{
    my $result = $soap->call(@_);
    
    my $err;
    if ($result->fault)
    {
        $err .= $result->faultdetail."\n"    if defined $result->faultdetail;
        $err .= $result->faultcode."\n"        if defined $result->faultcode;
        $err .= $result->faultstring."\n"    if defined $result->faultstring;
        $err .= $result->faultactor."\n"    if defined $result->faultactor;
    }

    return ($result->result, $err) if defined wantarray and wantarray;
    die $err if defined $err;
    return $result->result;
};

my $db = shift;
my $query = shift;

# Call the 'query' method of mrs with two parameters:
#     db:         the databank short name
#     queries:     this can be either a string or an array of strings consisting each
#                of one query to be performed.
#
# Result depends on the queries parameter. If queries is a single string the result
# will be an array of ids for the records that had an hit.
# If, however, the queries parameter is an array of strings the result will be
# an array of arrays of ids, one for each query

my $result = &soapCall(SOAP::Data->name("$ns:BlastSync")->attr({"xmlns:$ns" => $ns_url})
    => (
        SOAP::Data->name("$ns:db")        ->type('xsd:string' => $db),
        SOAP::Data->name("$ns:query")     ->type('xsd:string' => $query)
    ));

print Dumper($result);
