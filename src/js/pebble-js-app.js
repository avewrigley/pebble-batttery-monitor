
var url_root = "http://api.openweathermap.org/data/2.5/forecast?cnt=1&mode=json";
function fetchWeather( latitude, longitude ) 
{
    var response;
    var req = new XMLHttpRequest();
    var temp_units = localStorage.getItem( "temp_units" ) || "metric";
    var url = url_root + "&lat=" + latitude + "&lon=" + longitude + "&units=" + temp_units;
    console.log( "GET " + url );
    req.open( 'GET', url, true );
    req.onload = function(e) {
        if ( req.readyState == 4 ) 
        {
            if ( req.status == 200 ) 
            {
                // console.log( req.responseText );
                response = JSON.parse(req.responseText);
                if ( response && response.list && response.list.length > 0 ) 
                {
                    var city = response.city.name;
                    var weatherResult = response.list[0];
                    var temp = weatherResult.main.temp;
                    var description = weatherResult.weather[0].main;

                    console.log( "city: " + city );
                    console.log( "temp: " + temp );
                    console.log( "desc: " + description );
                    console.log( "temp_units: " + temp_units );
                    var transactionId = Pebble.sendAppMessage(
                        {
                            "temp": "" + temp,
                            "temp_units": "" + temp_units,
                            "city": "" + city,
                            "description": "" + description,
                        },
                        function( e ) {
                            console.log( "Successfully delivered message with transactionId=" + e.data.transactionId );
                        },
                        function( e ) {
                            console.log( "Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message );
                        }
                    );
                }
            } 
            else 
            {
                console.log( "Error" );
            }
        }
    }
    req.send( null );
}

function locationSuccess( pos ) 
{
    var coords = pos.coords;
    console.log( "locationSuccess" );
    var lat = "" + coords.latitude.toFixed( 3 );
    var lon = "" + coords.longitude.toFixed( 3 );
    console.log( "latitude: " + lat );
    console.log( "longitude: " + lon );
    console.log( "version 6" );
    var transactionId = Pebble.sendAppMessage(
        {
            "lat": lat,
            "lon": lon,
        },
        function( e ) {
            console.log( "Successfully delivered message with transactionId=" + e.data.transactionId );
        },
        function( e ) {
            console.log( "Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message );
        }
    );
    fetchWeather( coords.latitude, coords.longitude );
}

function locationError( err ) 
{
    console.warn( 'location error (' + err.code + '): ' + err.message );
}

function locationCallback()
{
    locationWatcher = window.navigator.geolocation.getCurrentPosition( locationSuccess, locationError, locationOptions );
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

Pebble.addEventListener(
    "ready",
    function(e) 
    {
        console.log( "ready: " + e.ready );
        // locationWatcher = window.navigator.geolocation.watchPosition( locationSuccess, locationError, locationOptions );
        locationCallback();
        window.setInterval( locationCallback, 60 * 60 * 1000 );
    }
);

Pebble.addEventListener(
    "showConfiguration",
    function(e) 
    {
        console.log( "showConfiguration" );
        var temp_units = localStorage.getItem( "temp_units" ) || "metric";
        var url = "http://ave.wrigley.name/pebble/pebble-batttery-monitor/configurable.html?temp_units=" + temp_units;
        // var url = "http://ave.wrigley.name/pebble/pebble-batttery-monitor/configurable.html";
        console.log( "GET: " + url );
        Pebble.openURL( url );
    }
);

Pebble.addEventListener(
    "webviewclosed",
    function( e ) 
    {
        console.log( "webview closed" );
        var options = JSON.parse( decodeURIComponent( e.response ) );
        console.log( options );
        for ( k in options )
        {
            console.log( "SET: " + k + " = " + options[k] );
            localStorage.setItem( k, options[k] );
            locationCallback();
        }
    }
);

