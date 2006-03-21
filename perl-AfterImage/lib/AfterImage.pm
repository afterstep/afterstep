package AfterImage;

use 5.005;
use strict;

require DynaLoader;
use vars qw($VERSION @ISA);
@ISA = qw(DynaLoader Class::Accessor);

use base qw(Class::Accessor);

$VERSION = '0.01';

bootstrap AfterImage $VERSION;

# $AfterImage::manager and $AfterImage::visual are hidden from the 
# user. They are both Singletons, and under no circumstances do we 
# allow the user to change their options.
#
# The manager has two potentially interesting abilities:
# 1. Controlling gamma.
# 2. Reference-counting images.
# Perl takes care of our reference counting, so we're left with gamma 
# as the only reason to expose the manager. This is non-compelling, 
# at least to me.
#
# The visual has the ability to support indexed images, which we 
# do not allow. Therefore there is no reason to expose the visual.
use vars qw($manager $visual);

my $defaults = {
	width      => 1,
	height     => 1,
	color      => '#00000000',
	drawing_op => 'alphablend',
	filename   => '',
};

AfterImage->mk_accessors(keys %$defaults);
AfterImage->mk_ro_accessors(qw(width height color filename));

sub new {
	my $class = shift;
	my $self  = {};
	my $options = defined($_[0]) && ref($_[0]) eq 'HASH' ? $_[0] : { @_ };
	$AfterImage::manager ||= AfterImage::c_manager_create('');
	$AfterImage::visual ||= AfterImage::c_visual_create();
	foreach (keys %$defaults) {
		if (exists $$options{$_}) {
			$self->{$_} = $$options{$_};
		} else {
			$self->{$_} = $$defaults{$_};
		}
	}
	bless($self, $class);
	if ($self->filename) {
		my $loaded_image = AfterImage::c_load($AfterImage::manager, $self->filename);
		$self->{width} = AfterImage::c_width($loaded_image);
		$self->{height} = AfterImage::c_height($loaded_image);
		$self->{image} = $loaded_image;
	} else {
		$self->{image} = AfterImage::c_solid($visual, $self->width, $self->height, $self->{color});
	}
	return $self;
}

sub clone {
	my $self = shift;
	my $new_image = { %$self };
	$self->{image} = AfterImage::c_clone($self->{image});
	bless($new_image, ref($self));
	return $self;
}

sub draw_filled_rectangle {
	my $self = shift;
	my ($color, $x, $y, $width, $height) = @_;
	my $visual = $AfterImage::visual;
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);
	AfterImage::c_draw_filled_rectangle($visual, $self->{image}, $x, $y, $width, $height, $color);
	return $self;
}

sub draw_simple_gradient {
	my $self = shift;
	my ($angle, $colors, $x, $y, $width, $height) = @_;
	if (!$colors || ref($colors) ne 'ARRAY' || !@$colors) {
		die "\$colors must be a non-empty arrayref\n";
	}
	if (@$colors < 2) {
		$colors = [ $$colors[0], $$colors[0] ];
	}
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);
	my $offset_and_color = [];
	for (my $i = 0 ; $i < @$colors ; $i++) {
		push(@$offset_and_color, {
			color  => $$colors[$i], 
			offset => $i / (@$colors - 1),
		});
	}
	return $self->draw_gradient($angle, $offset_and_color, $x, $y, $width, $height);
}

sub draw_gradient {
	my $self = shift;
	my ($angle, $offset_and_color, $x, $y, $width, $height) = @_;
	if (!$offset_and_color || ref($offset_and_color) ne 'ARRAY' || !@$offset_and_color) {
		die "\$offset_and_color must be a non-empty arrayref\n";
	}
	if (@$offset_and_color < 2) {
		@$offset_and_color = [ $$offset_and_color[0], $$offset_and_color[0] ];
	}
	foreach (@$offset_and_color) {
		if (!exists $$_{color} || !exists $$_{offset}) {
			die "\$offset_and_color is missing a color or offset\n";
		}
	}
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);

	# Extract the offsets and colors.
	my @colors = map { $$_{color} } @$offset_and_color;
	my @offsets = map { $$_{offset} } @$offset_and_color;

	my $gradient = AfterImage::c_gradient(
		$visual, $width, $height, \@colors, \@offsets, $angle
	);
	$self->composite_and_replace_current_image($gradient, $x, $y);

	return $self;
}

sub draw_image {
	my $self = shift;
	my ($image, $x, $y, $width, $height) = @_;
	($x, $y, $width, $height) = $image->autofill_position($x, $y, $width, $height);

	$self->composite_and_replace_current_image($image->{image}, $x, $y);

	return $self;
}

sub draw_text {
	my $self = shift;
	my ($font, $text, $color, $x, $y) = @_;
	($x, $y) = $self->autofill_position($x, $y, 0, 0);

	my $text_image = AfterImage::c_draw_text(
		$AfterImage::visual, $font->{font}, $text, $color, 0
	);
	$self->composite_and_replace_current_image($text_image, $x, $y);

	return $self;
}

sub scale {
	my $self = shift;
	my ($width, $height) = @_;
	my ($x, $y);
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);

	if ($width != $self->width || $height != $self->height) {
		my $scaled_image = AfterImage::c_scale(
			$AfterImage::visual, $self->{image}, $width, $height
		);
		$self->{width} = $width;
		$self->{height} = $height;
		$self->replace_current_image($scaled_image);
	}

	return $self;
}

sub resize {
	my $self = shift;
	my ($x, $y, $width, $height) = @_;
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);

	if ($x || $y || $width != $self->width || $height != $self->height) {
		my $resized_image = AfterImage::c_pad(
			$AfterImage::visual, $self->{image}, $x, $y, $width, $height
		);
		$self->{width} = $width;
		$self->{height} = $height;
		$self->replace_current_image($resized_image);
	}

	return $self;
}

sub tile {
	my $self = shift;
	my ($x, $y, $width, $height) = @_;
	($x, $y, $width, $height) = $self->autofill_position($x, $y, $width, $height);

	if ($width != $self->width || $height != $self->height) {
		my $tiled_image = AfterImage::c_tile(
			$AfterImage::visual, $self->{image}, $x, $y, $width, $height
		);
		$self->{width} = $width;
		$self->{height} = $height;
		$self->replace_current_image($tiled_image);
	}

	return $self;
}

sub gaussian_blur {
	my $self = shift;
	my ($x_radius, $y_radius) = @_;

	if (0) {
	my $blurred_image = AfterImage::c_gaussian_blur(
		$AfterImage::visual, $self->{image}, $x_radius, $y_radius
	);
	$self->replace_current_image($blurred_image);
	} else {
	print "gaussian_blur() is currently broken in libAfterImage.\n";
	}

	return $self;
}

sub tint {
	my $self = shift;
	my ($color) = @_;

	if ($color ne '#7f7f7f7f') {
		my $tinted_image = AfterImage::c_tint(
			$AfterImage::visual, $self->{image}, $color
		);
		$self->replace_current_image($tinted_image);
	}

	return $self;
}

sub rotate {
	my $self = shift;
	my ($angle) = @_;

	my $rotated_image = AfterImage::c_rotate(
		$AfterImage::visual, $self->{image}, $angle
	);
	$self->replace_current_image($rotated_image);

	return $self;
}

sub mirror_horizontal {
	my $self = shift;

	my $mirrored_image = AfterImage::c_mirror(
		$AfterImage::visual, $self->{image}, 0
	);
	$self->replace_current_image($mirrored_image);

	return $self;
}

sub mirror_vertical {
	my $self = shift;

	my $mirrored_image = AfterImage::c_mirror(
		$AfterImage::visual, $self->{image}, 1
	);
	$self->replace_current_image($mirrored_image);

	return $self;
}

sub load {
	my $self = shift;
	my ($filename) = @_;
	my $loaded_image = AfterImage::c_load($AfterImage::manager, $filename);
	$self->{filename} = $filename;
	$self->{width} = AfterImage::c_width($loaded_image);
	$self->{height} = AfterImage::c_height($loaded_image);
	$self->replace_current_image($loaded_image);
}

sub save {
	my $self = shift;
	my ($filename, $quality, $file_type) = @_;
	if (!$file_type) {
		($file_type) = $filename =~ m{([^.]+)$}o;
		if (!$file_type) {
			die "unable to determine file type from filename\n";
		}
		$file_type = lc $file_type;
	}
	$quality = 100 if !defined $quality;
	if ($file_type ne 'jpg' && $file_type ne 'jpeg') {
		$quality = 100;
	}
	AfterImage::c_save($self->{image}, $filename, $file_type, $quality, 100);
}

# PRIVATE functions.

sub DESTROY {
	my $self = shift;
	AfterImage::c_release($self->{image});
}

sub composite_and_replace_current_image {
	my $self = shift;
	my ($image, $x, $y) = @_;
	my $visual = $AfterImage::visual;
	my $new_image = AfterImage::c_composite($visual, 
		[ $image, $self->{image} ], $self->width, $self->height, 
		$self->drawing_op, [ { x => $x, y => $y } ]
	);
	$self->replace_current_image($new_image);
}

sub replace_current_image {
	my $self = shift;
	my $new_image = shift;
	AfterImage::c_release($self->{image});
	$self->{image} = $new_image;
}

sub autofill_position {
	my $self = shift;
	my ($x, $y, $width, $height) = @_;
	return (
		($x || 0), 
		($y || 0), 
		($width || $self->width),
		($height || $self->height),
	);
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
