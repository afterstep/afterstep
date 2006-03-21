#!/usr/bin/perl -w

use strict;

use ExtUtils::testlib;
use AfterImage;

my $image = new AfterImage { width => 100, height => 100 };
$image->draw_simple_gradient(90, [ "#ffff0000", "#ff00ff00", "#ff0000ff" ]);
$image->draw_solid_rectangle("#7fffff00", 10, 10, 80, 80);
$image->draw_solid_rectangle("#7f7f7f7f", 25, 25, 50, 50);
$image->save("output.png");

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
