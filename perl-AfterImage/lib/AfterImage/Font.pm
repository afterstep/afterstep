package AfterImage::Font;

use 5.005;
use strict;

require DynaLoader;
use vars qw($VERSION @ISA);
@ISA = qw(DynaLoader Class::Accessor);

use base qw(Class::Accessor);

$VERSION = '0.01';

bootstrap AfterImage $VERSION;

# $AfterImage::Font::manager is hidden from the user. It is a 
# Singleton, and under no circumstances do we allow the user to 
# change its options.
use vars qw($manager);

my $defaults = {
	filename   => '',
	points     => 0,
};

AfterImage::Font->mk_accessors(keys %$defaults);
AfterImage::Font->mk_ro_accessors(qw(filename));

sub new {
	my $class = shift;
	my $self  = {};
	my $options = defined($_[0]) && ref($_[0]) eq 'HASH' ? $_[0] : { @_ };
	$AfterImage::Font::manager ||= AfterImage::Font::c_manager_create($ENV{ASFONT_PATH_ENVVAR} || '');
	foreach (keys %$defaults) {
		if (exists $$options{$_}) {
			$self->{$_} = $$options{$_};
		} else {
			$self->{$_} = $$defaults{$_};
		}
	}
	bless($self, $class);
	if (!$self->filename) {
		die "AfterImage::Font::new(): font name or filename required\n";
	}
	if (!$self->points) {
		die "AfterImage::Font::new(): font size required\n";
	}
	$self->{font} = AfterImage::Font::c_open($AfterImage::Font::manager, $self->filename, $self->points);
	return $self;
}

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

AfterImage - Perl extension for blah blah blah

=head1 SYNOPSIS

  use AfterImage;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for AfterImage, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.



=head1 SEE ALSO

Mention other useful documentation such as the documentation of
related modules or operating system documentation (such as man pages
in UNIX), or any relevant external documentation such as RFCs or
standards.

If you have a mailing list set up for your module, mention it here.

If you have a web site set up for your module, mention it here.

=head1 AUTHOR

Ethan Fischer, E<lt>allanon@crystaltokyo.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Ethan Fischer

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.8 or,
at your option, any later version of Perl 5 you may have available.


=cut
