
var icons = {
    [% FOREACH image IN images %]
    "[% image.name %]": [% loop.index %][% UNLESS loop.last %],[% END %]
    [% END %]
};

// var url_root = "http://api.openweathermap.org/data/2.5/forecast?cnt=1&mode=json";
var url_root = "http://api.openweathermap.org/data/2.5/weather?cnt=1&mode=json";
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
                response = JSON.parse(req.responseText);
                if ( response )
                {
                    var city = response.name;
                    console.log( "city: " + city );
                    var weather = response.weather[0];
                    console.log( weather );
                    var description = weather.description;
                    console.log( "desc: " + description );
                    var icon = weather.icon;
                    console.log( "icon: " + icon );
                    var icon_no = icons[icon];
                    console.log( "icon_no: " + icon_no );
                    var temp = response.main.temp;
                    console.log( "temp: " + temp );
                    console.log( "temp_units: " + temp_units );
                    var transactionId = Pebble.sendAppMessage(
                        {
                            "temp": "" + temp,
                            "temp_units": temp_units,
                            "city": "" + city,
                            "description": "" + description,
                            "icon": icon_no,
                        },
                        function( e ) {
                            console.log( "Successfully delivered weather message with transactionId=" + e.data.transactionId );
                        },
                        function( e ) {
                            console.log( "Unable to deliver weather message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message );
                        }
                    );
                    console.log( "Sent weather message with transactionId=" + transactionId );
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
    var lat = "" + coords.latitude.toFixed( 3 );
    var lon = "" + coords.longitude.toFixed( 3 );
    console.log( "latitude: " + lat );
    console.log( "longitude: " + lon );
    var transactionId = Pebble.sendAppMessage(
        {
            "lat": lat,
            "lon": lon,
        },
        function( e ) {
            console.log( "Successfully delivered location message with transactionId=" + e.data.transactionId );
        },
        function( e ) {
            console.log( "Unable to deliver location message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message );
        }
    );
    console.log( "Sent location message with transactionId=" + transactionId );
    fetchWeather( coords.latitude, coords.longitude );
}

function locationError( err ) 
{
    console.warn( 'location error (' + err.code + '): ' + err.message );
}

function getLocation()
{
    window.navigator.geolocation.getCurrentPosition( locationSuccess, locationError, locationOptions );
}

function configUpdate()
{
    var clock_format = localStorage.getItem( "clock_format" ) || "12h";
    console.log( "clock_format: " + clock_format );
    var transactionId = Pebble.sendAppMessage(
        {
            "clock_format": "" + clock_format,
        },
        function( e ) {
            console.log( "Successfully delivered config message with transactionId=" + e.data.transactionId );
        },
        function( e ) {
            console.log( "Unable to deliver config message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message );
        }
    );
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

var intvar;

function setWeatherCallback()
{
    var weather_interval = localStorage.getItem( "weather_interval" ) || 10;
    console.log( "weather_interval: " + weather_interval );
    if ( intvar ) window.clearInterval( intvar );
    intvar = window.setInterval( getLocation, weather_interval * 60 * 1000 );
}

Pebble.addEventListener(
    "ready",
    function(e) 
    {
        setWeatherCallback();
        getLocation();
        configUpdate();
    }
);

Pebble.addEventListener(
    "showConfiguration",
    function(e) 
    {
        var temp_units = localStorage.getItem( "temp_units" ) || "metric";
        var clock_format = localStorage.getItem( "clock_format" ) || "12h";
        var weather_interval = localStorage.getItem( "weather_interval" ) || "10";
        var url = 
            "http://ave.wrigley.name/pebble/pebble-batttery-monitor/configurable.html?" + 
                "temp_units=" + temp_units + 
                "&clock_format=" + clock_format +
                "&weather_interval=" + weather_interval
        ;
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
        for ( k in options )
        {
            console.log( "SET: " + k + " = " + options[k] );
            localStorage.setItem( k, options[k] );
        }
        setWeatherCallback();
        getLocation();
        configUpdate();
    }
);
Pebble.addEventListener(
    "appmessage",
    function(e) 
    {
        console.log( "message" );
        if ( e.payload.fetch_weather )
        {
            getLocation();
        }
    }
);
