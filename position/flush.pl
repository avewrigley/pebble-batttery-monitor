#!/usr/bin/perl

use Redis;
use Digest::MD5 qw( md5_hex );

use strict;
use warnings;

my $max_markers = 10;

my $redis = Redis->new;
die "failed to connect to redis server" unless $redis;
for my $id ( $redis->keys( '*' ) )
{
    my $type = $redis->type( $id );
    warn "ID $id: TYPE $type";
    if ( $type eq 'list' )
    {
        my $id_hash = md5_hex( $id );
        my @values = $redis->lrange( $id, 0, $max_markers );
        if ( @values )
        {
            warn "ID $id: GOT @values";
        }
        warn "id_hash: $id_hash";
        @values = $redis->lrange( $id_hash, 0, $max_markers );
        if ( @values )
        {
            warn "ID HASH $id_hash: GOT @values";
            $redis->del( $id );
        }
    }
    else
    {
        warn "ID $id: unexpected type: $type";
    }
}
