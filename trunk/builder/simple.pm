package MRS::Parser::simple;

@ISA = qw( MRS::Parser );

sub new
{
	my $invocant = shift;

	my $self = new MRS::Parser(
		'name'	=> "Dit is mijn naam",
		'adres'	=> "En dit is mijn adres",
		@_
	);

	return bless $self, "MRS::Parser::simple";
}

sub Parse
{
	my ($self) = @_;

	my $doc = "";
	
	while (my $line = $self->GetLine())
	{
		$doc .= $line;
		chomp($line);

		my ($label, $value) = split(m/\t/, $line);

		if ($label eq '//')
		{
			$self->Store($doc);
			$self->FlushDocument();
			$doc = "";
		}
		elsif ($label eq 'ID')
		{
			$self->IndexValue('id', $value);
		}
		else
		{
			$self->IndexText($label, $value);
		}
	}
}

1;
