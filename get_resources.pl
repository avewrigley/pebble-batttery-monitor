#!/usr/bin/perl

use strict;
use warnings;
use Template;
use Image::Size;

my @images;
for my $d ( qw( d n ) )
{
    for my $n ( map { sprintf( "%02d", $_ ) }  ( 1 .. 4, 9 .. 11, 13, 50 ) )
    {
        my $image_file = "resources/img/${n}${d}.png";
        my $cmd = "lwp-request -m GET http://openweathermap.org/img/w/${n}${d}.png > $image_file";
        warn $cmd;
        `$cmd`;
        my ( $x, $y ) = imgsize( $image_file );
        push( @images, { file => "img/${n}${d}.png", name => "${n}${d}", x => $x, y => $y } );
    }
}
my $template = new Template;
$template->process( "templates/appinfo.json", { images => \@images }, "appinfo.json" ) || die $template->error();
$template->process( "templates/battery-monitor.c", { images => \@images }, "src/battery-monitor.c", ) || die $template->error();
$template->process( "templates/pebble-js-app.js", { images => \@images }, "src/js/pebble-js-app.js", ) || die $template->error();
