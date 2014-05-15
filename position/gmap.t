<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="initial-scale=1.0, user-scalable=no" />
    <style type="text/css">
      html { height: 100% }
      body { height: 100%; margin: 0; padding: 0 }
      #map-canvas { height: 100% }
    </style>
    <script type="text/javascript"
      src="https://maps.googleapis.com/maps/api/js?key=[% api_key %]">
    </script>
    <script type="text/javascript">

var map;
var directionsDisplay = new google.maps.DirectionsRenderer();
var directionsService = new google.maps.DirectionsService();
var bounds = new google.maps.LatLngBounds();

function calcRoute( start, end, wp ) 
{
    var request;
    if ( ! end )
    {
        console.log( "only one marker" );
        return;
    }
    else if ( wp.length === 0 )
    {
        console.log( "only two markers" );
        request = {
            origin: start,
            destination: end,
            travelMode: google.maps.TravelMode.DRIVING
        };
    }
    else
    {
        request = {
            origin: start,
            destination: end,
            waypoints: wp,
            travelMode: google.maps.TravelMode.DRIVING
        };
    }
    console.log( request );
    directionsService.route( 
        request, 
        function( result, status ) 
        {
            if ( status == google.maps.DirectionsStatus.OK ) {
                directionsDisplay.setDirections( result );
            }
        }
    );
}

function initialize() 
{
    map = new google.maps.Map( document.getElementById( "map-canvas" ), { zoom: [% zoom %] } );
    directionsDisplay.setMap( map );
    var markers = [
    [% FOREACH marker IN markers %]
        new google.maps.Marker( {
            position: new google.maps.LatLng( [% marker.lat %], [% marker.lon %] ),
            title: "[% id %] @ [% marker.t %]"
        } )[% UNLESS loop.last %],[% END %]
    [% END %]
    ];
    var start, end;
    var waypoints = [];
    for ( var i = 0; i < markers.length; i++ )
    {
        var marker = markers[i];
        var latLon = marker.getPosition();
        if ( i === 0 ) start = latLon;
        else if ( i === markers.length-1 ) end = latLon;
        else waypoints.push( { location: latLon } );
        bounds.extend( latLon );
        marker.setMap( map );
        map.fitBounds( bounds );
    }
    calcRoute( start, end, waypoints );
}

google.maps.event.addDomListener( window, 'load', initialize );

    </script>
  </head>
  <body>
    <div id="map-canvas"/>
  </body>
</html>

