#!/usr/bin/perl -w

use strict;

use ExtUtils::testlib;
use AfterImage;

my $m = AfterImage::imagemanager_create('');
my $im = [
	AfterImage::image_solid(50, 50, "#7f7f7f7f"),
	AfterImage::image_solid(80, 80, "#7fffff00"),
	AfterImage::image_gradient(
		100, 100, [ "#ffff0000", "#ff00ff00", "#ff0000ff" ], [ 0, 0.5, 1 ], 90
	),
];
my $cim = AfterImage::image_composite($im, 100, 100, "alphablend", [
	{ x => 25, y => 25 }, 
	{ 
		x => 10, y => 10, 
		clip_x => 10, clip_y => 10, clip_width => 40, clip_height => 40, 
		tint => '#7f00ff00'
	},
]);
AfterImage::image_save($cim, "output.png", "png", 99, 100);
