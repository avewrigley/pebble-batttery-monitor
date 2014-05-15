
var icons = {
    [% FOREACH image IN images %]
    "[% image.name %]": [% loop.index %][% UNLESS loop.last %],[% END %]
    [% END %]
};

var weather_url_root = "http://api.openweathermap.org/data/2.5/forecast?mode=json";
var position_url_root = "http://www.batterymon.co.uk/position?";
var config_url_root = "http://www.batterymon.co.uk/configurable.html?";
var days = [ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" ];

function fetchWeather( latitude, longitude ) 
{
    var response;
    var req = new XMLHttpRequest();
    var temp_units = localStorage.getItem( "temp_units" ) || "metric";
    var url = weather_url_root + "&lat=" + latitude + "&lon=" + longitude + "&units=" + temp_units;
    console.log( "GET " + url );
    req.open( 'GET', url, true );
    req.onload = function(e) {
        if ( req.readyState == 4 ) 
        {
            if ( req.status == 200 ) 
            {
                response = JSON.parse( req.responseText );
                if ( response )
                {
                    var city = response.city.name;
                    console.log( "city: " + city );
                    var description = [];
                    var icon_no = [];
                    var temp = [];
                    var time = [];
                    var datestamp = [];
                    for ( var i = 0; i < 5; i++ )
                    {
                        var forecast = response.list[i];
                        var weather = forecast.weather[0];
                        description.push( weather.description );
                        var icon = weather.icon;
                        icon_no.push( icons[icon] );
                        temp.push( forecast.main.temp.toFixed(1) );
                        var dt = forecast.dt;
                        var date = new Date( dt * 1000 );
                        var h = date.getHours().toString();
                        if ( h.length == 1 )  h = "0" + h;
                        var s = date.getSeconds().toString();
                        if ( s.length == 1 )  s = "0" + s;
                        var time = h + ":" + s;
                        var day = days[date.getDay()];
                        datestamp.push( day + " " + time );
                        console.log( "description = " + weather.description + ", icon = " + icons[icon] + ", datestamp = " + day + " " + time + ", temp = " + forecast.main.temp );
                    }
                    var transactionId = Pebble.sendAppMessage(
                        {
                            "temp": "" + temp.join( ";" ),
                            "temp_units": temp_units,
                            "city": "" + city,
                            "description": "" + description.join( ";" ),
                            "icon": icon_no.join( ";" ),
                            "datestamp": datestamp.join( ";" )
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
                console.log( "weather service error" );
            }
        }
    }
    req.send( null );
}

function logPosition( latitude, longitude ) 
{
    var req = new XMLHttpRequest();
    var id = Pebble.getAccountToken();
    var t = Math.round(+new Date()/1000);
    var url = position_url_root + "&lat=" + latitude + "&lon=" + longitude + "&id=" + id + "&t=" + t;
    console.log( "GET " + url );
    req.open( 'GET', url, true );
    req.onload = function(e) {
        if ( req.readyState == 4 ) 
        {
            if ( req.status == 200 ) 
            {
                console.log( "position service success" );
            } 
            else 
            {
                console.log( "position service error" );
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
    var track = localStorage.getItem( "track" ) || "n";
    if ( track === "y" )
    {
        logPosition( coords.latitude, coords.longitude );
    }
    else
    {
        console.log( "DO NOT TRACK" );
    }
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
        localStorage.setItem( "forecast_no", 0 );
    }
);

Pebble.addEventListener(
    "showConfiguration",
    function(e) 
    {
        var temp_units = localStorage.getItem( "temp_units" ) || "metric";
        var clock_format = localStorage.getItem( "clock_format" ) || "12h";
        var weather_interval = localStorage.getItem( "weather_interval" ) || "10";
        var track = localStorage.getItem( "track" ) || "n";
        var id = Pebble.getAccountToken();
        var url = 
            config_url_root + 
            "track=" + track + 
            "&temp_units=" + temp_units + 
            "&clock_format=" + clock_format +
            "&weather_interval=" + weather_interval +
            "&id=" + id
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
        if ( e.payload.fetch_weather )
        {
            getLocation();
        }
    }
);
