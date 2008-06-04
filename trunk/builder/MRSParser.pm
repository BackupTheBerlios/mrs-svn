# base class definition for MRS Parser objects

package MRS;

use strict;

sub valid_package_name
{
	my($string) = @_;
	$string =~ s/([^A-Za-z0-9\/])/sprintf("_%2x",unpack("C",$1))/eg;
	# second pass only for words starting with a digit
	$string =~ s|/(\d)|sprintf("/_%2x",unpack("C",$1))|eg;
	
	# Dress it up as a real package name
	$string =~ s|/|::|g;
	return "Embed" . $string;
}

sub load_parser
{
	my ($filename) = @_;

	my $package = valid_package_name($filename);

	my $fh;
	open($fh, "$filename.pm") or die "open '$filename' $!";
	local($/) = undef;
	my $sub = <$fh>;
	close $fh;
	
	#wrap the code into a subroutine inside our unique package
	my $eval = qq{package $package; sub handler { $sub; }};
	{
		# hide our variables within this block
		my ($filename,$package,$sub);
		eval $eval;
	}
	die $@ if $@;

	eval {
		$package->handler;
	};
	die $@ if $@;

	my $parser_name = "MRS::Parser::${filename}";
	return new $parser_name;
}

package MRS::Parser;

use strict;

require Exporter;
require DynaLoader;

our @ISA = qw(Exporter DynaLoader);
our $VERSION = '0.1';

our @EXPORT = qw( );

sub TIEHASH
{
	my ($classname, $obj) = @_;
	return bless $obj, $classname;
}

sub this
{
	my $ptr = shift;
	return tied(%$ptr);
}

1;

__END__
