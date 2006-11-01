#!perl

package Embed::WSFormat;

sub pretty
{
	my ($parser, $text) = @_;

	print STDERR "\n\n***\n\nDit komt uit mijn pretty perl routine voor $parser\n";
	
	return "<pre>parser: $parser, text: $text</pre>";
}

1;
