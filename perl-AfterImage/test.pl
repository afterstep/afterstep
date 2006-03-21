#!/usr/bin/perl -w

use strict;

use ExtUtils::testlib;
use Time::HiRes qw(&time);
use AfterImage;
use AfterImage::Font;

my $start_stamp = time;
my $font = new AfterImage::Font { filename => '../test/vera_sans.ttf', points => 24 };
# "Satori" is an image from digitalblasphemy.com:
# http://digitalblasphemy.com/dbgallery/1/satori.shtml
my $satori = new AfterImage { filename => '../test/satori.jpg' };
$satori->scale(50, 50);
$satori->resize(25, 25, 100, 100);
$satori->tint('#6f7f7f7f');
$satori->mirror_horizontal();
my $image = new AfterImage { width => 100, height => 100 };
$image->draw_simple_gradient(90, [ "#ffff0000", "#ff00ff00", "#ff0000ff" ]);
$image->draw_filled_rectangle("#7fffff00", 10, 10, 80, 80);
$image->draw_filled_rectangle("#7f7f7f7f", 25, 25, 50, 50);
$image->scale(50, 50);
$image->tile(0, 0, 100, 100);
$image->rotate(180);
$image->draw_image($satori, 0, 0);
$image->gaussian_blur(1.2, 1.2);
$image->draw_text($font, "AS", "#ff000000", 35, 75);
$image->save("output.png");
printf("Completed in %.4f seconds.\n", time - $start_stamp);

__END__

my $m = AfterImage::c_manager_create('');
my $v = AfterImage::c_visual_create();
my $im = [
	AfterImage::c_solid($v, 50, 50, "#7f7f7f7f"),
	AfterImage::c_solid($v, 80, 80, "#7fffff00"),
	AfterImage::c_gradient($v, 
		100, 100, [ "#ffff0000", "#ff00ff00", "#ff0000ff" ], [ 0, 0.5, 1 ], 90
	),
];
my $cim = AfterImage::c_composite($v, $im, 100, 100, "alphablend", [
	{ x => 25, y => 25 }, 
	{ 
		x => 10, y => 10, 
		clip_x => 10, clip_y => 10, clip_width => 40, clip_height => 40, 
		tint => '#7f00ff00'
	},
]);
AfterImage::c_save($cim, "output.jpg", "jpg", 1, 100);
