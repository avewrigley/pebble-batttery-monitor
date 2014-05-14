use Plack::Request;
use POSIX qw( strftime );
use Redis;
use Template;
use YAML qw( LoadFile );

use strict;
use warnings;

my $configfile = "/etc/pebblepos/pebblepos.conf";
-e $configfile or die "no config file $configfile";
my $config = LoadFile( $configfile ) or die "can't parse $configfile";
my $api_key = $config->{api_key} or die "no api_key";
my $template_path = $config->{template_path} or die "no template_path";
sub {
    my $req = Plack::Request->new( shift );
    my $code = 200;
    my $id = $req->param( 'id' );
    my $res = $req->new_response( $code );
    $res->content_type( "text/html" );
    my $redis = Redis->new;
    die "failed to connect top redis server" unless $redis;
    if ( $id )
    {
        my $lon = $req->param( 'lon' );
        my $lat = $req->param( 'lat' );
        my $t = $req->param( 't' );
        if ( $lat && $lon )
        {
            warn "$id: $lat, $lon, $t";
            $redis->set( $id => "$lat,$lon,$t" );
        }
        else
        {
            my $value = $redis->get( $id );
            ( $lat, $lon, $t ) = split( ',', $value );
            warn "$id: $lat, $lon";
            my $output;
            my $template = Template->new( ABSOLUTE => 1 );
            my $ts = strftime( "%c", localtime( $t ) );
            $template->process( $template_path, { api_key => $api_key, zoom => 15, id => $id, ts => $ts, lon => $lon, lat => $lat }, \$output ) || die $template->error();
            $res->body( $output );
        }
    }
    return $res->finalize;
}
