use Plack::Request;
use POSIX qw( strftime );
use Redis;
use Template;
use YAML qw( LoadFile );
use Digest::MD5 qw( md5_hex );

use Log::Any qw( $log );
use Log::Dispatch;
use Log::Dispatch::FileRotate;
use Log::Any::Adapter;
use FindBin qw( $Bin );
use strict;
use warnings;

my $logfile = '/var/log/pebble-battery-monitor/pebblepos.log';
my $max_markers = 10;

my $dispatcher = Log::Dispatch->new(
    callbacks  => sub {
        my %args = @_;
        my $message = $args{message};
        return "$$: " . uc( $args{level} ) . ": " . scalar( localtime ) . ": $message";
    }
);

Log::Any::Adapter->set( 'Dispatch', dispatcher => $dispatcher );

$dispatcher->add(
    my $file = Log::Dispatch::FileRotate->new(
        name            => 'logfile',
        min_level       => 'debug',
        filename        => $logfile,
        mode            => 'append' ,
        DatePattern     => 'yyyy-MM-dd',
        max             => 7,
        newline         => 1,
    ),
);

my $configfile = "/etc/pebblepos/pebblepos.conf";
-e $configfile or die "no config file $configfile";
my $config = LoadFile( $configfile ) or die "can't parse $configfile";
my $api_key = $config->{api_key} or die "no api_key";

$log->debug( "restart $0 ($$) in $Bin" );

sub {
    my $req = Plack::Request->new( shift );
    my $code = 200;
    my $id = $req->param( 'id' );
    my $res = $req->new_response( $code );
    $res->content_type( "text/html" );
    my $redis = Redis->new;
    die "failed to connect to redis server" unless $redis;
    if ( $id )
    {
        $log->debug( "id = $id" );
        my $id_hash = md5_hex( $id );
        $log->debug( "id_hash = $id_hash" );
        my $lon = $req->param( 'lon' );
        my $lat = $req->param( 'lat' );
        my $t = $req->param( 't' );
        if ( $lat && $lon && $t )
        {
            $log->debug( "$id_hash: SET $lat, $lon, $t" );
            $redis->lpush( $id_hash => "$lat,$lon,$t" );
            $redis->ltrim( $id_hash, 0, $max_markers );
        }
        else
        {
            $log->debug( "GET $id_hash markers" );
            my @values = $redis->lrange( $id_hash, 0, $max_markers );
            if ( @values )
            {
                $log->debug( "$id_hash: GOT @values" );
                my @markers;
                for my $value ( @values )
                {
                    ( $lat, $lon, $t ) = split( ',', $value );
                    my $ts = strftime( "%c", localtime( $t ) );
                    $log->debug( "$id_hash: GET $lat, $lon, $ts" );
                    my $marker = { lat => $lat, lon => $lon, t => $ts };
                    push( @markers, $marker );
                }
                my $output;
                my $template = Template->new( { INCLUDE_PATH => $Bin } );
                my $ts = strftime( "%c", localtime( $t ) );
                $template->process( 
                    "gmap.t",
                    { api_key => $api_key, zoom => 15, id => $id_hash, markers => \@markers },
                    \$output 
                ) || die $template->error();
                $res->body( $output );
            }
        }
    }
    return $res->finalize;
}
