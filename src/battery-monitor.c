// Standard includes
#include "pebble.h"


// App-specific data
Window *window; // All apps must have at least one window

Layer *root_layer;
GRect frame;

TextLayer *time_layer; 
TextLayer *am_layer; 
TextLayer *secs_layer; 
TextLayer *weather_layer; 
TextLayer *date_layer; 
TextLayer *battery_layer;
TextLayer *connection_layer;
TextLayer *notify_layer; 

static GBitmap *weather_bitmap[18];

static BitmapLayer *weather_icon_layer;

static GBitmap *battery_bitmap = NULL;
static BitmapLayer *battery_icon_layer;

const uint32_t inbound_size = 256;
const uint32_t outbound_size = 256;

#define BATTERY_LEVEL 1
static int battery_level = 100;
static char clock_format[] = "12h";

enum GeoKey {
    LAT = 0x0,
    LON = 0x1,
    TEMP = 0x2,
    CITY = 0x3,
    DESCRIPTION = 0x4,
    TEMP_UNITS = 0x5,
    CLOCK_FORMAT = 0x6,
    ICON = 0x7,
    FETCH_WEATHER = 0x8,
    DATESTAMP = 0x9
};

static AppTimer *timer;

#define NO_FORECASTS 5
static int cycle_secs = 4;
char *description[NO_FORECASTS];
char *temperature[NO_FORECASTS];
char *icon[NO_FORECASTS];
char *datestamp[NO_FORECASTS];
char *temp_units = " C";
int weather_no = 0;

char *weather_str;
char *geo_str;

static void notify_timer_callback( void *data ) 
{
    layer_set_hidden( text_layer_get_layer( notify_layer ), true );
}

static void notify( char *text )
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "notify '%s'", text );
    layer_set_hidden( text_layer_get_layer( notify_layer ), false );
    text_layer_set_text( notify_layer, text );
    timer = app_timer_register( 5000 /* milliseconds */, notify_timer_callback, NULL );
}

void out_sent_handler( DictionaryIterator *sent, void *context ) 
{
}


void out_failed_handler( DictionaryIterator *failed, AppMessageResult reason, void *context ) 
{
}

char* p_start = NULL; /* the pointer to the start of the current fragment */
char* p_end = NULL; /* the pointer to the end of the current fragment */

char* strtok1( char* str, const char delimiter ) 
{
    if ( str )
    {
        p_start = str;
    }
    else
    {
        if ( p_end == NULL ) return NULL;
        p_start = p_end+1;
    }
    for ( p_end = p_start; true; p_end++ )
    {
        if ( *p_end == '\0' )
        {
            p_end = NULL;
            break;
        }
        if ( *p_end == delimiter )
        {
            *p_end = '\0';
            break;
        }
    }
    return p_start;
}

void set_array( char **array, char *str ) 
{
    int i, len;
    char *tok, *text;
    len = strlen( str ) + 1;
    text = malloc( len );
    strcpy( text, str );
    for ( ( i = 0, tok = strtok1( text, ';' ) ); tok != NULL && i < NO_FORECASTS; ( i++, tok = strtok1( NULL, ';' ) ) )
    {
        len = strlen( tok );
        char *str = malloc( len+1 );
        strcpy( str, tok );
        if ( array[i] != NULL ) free( array[i] );
        array[i] = str;
    }
    if ( i == NO_FORECASTS+1 )
    {
        APP_LOG( APP_LOG_LEVEL_WARNING, "%d excedes no of forecasts(%d)", i, NO_FORECASTS );
    }
    free( text );
}

static void cycle_weather()
{
    if ( datestamp[weather_no] == NULL )
    {
        // APP_LOG( APP_LOG_LEVEL_DEBUG, "no forecast no. %i", weather_no );
        return;
    }
    if ( datestamp[weather_no] != NULL && description[weather_no] != NULL && temperature[weather_no] != NULL )
    {
        // APP_LOG( APP_LOG_LEVEL_DEBUG, "desc: %s", description[weather_no] );
        int len = strlen( geo_str ) + strlen( datestamp[weather_no] ) + strlen( description[weather_no] ) + strlen( temperature[weather_no] ) + strlen( temp_units ) + 4;
        if ( weather_str != NULL )
        {
            free( weather_str );
        }
        weather_str = malloc( len );
        strcpy( weather_str, geo_str );
        strcat( weather_str, "\n" );
        strcat( weather_str, datestamp[weather_no] );
        strcat( weather_str, "\n" );
        strcat( weather_str, description[weather_no] );
        strcat( weather_str, "\n" );
        strcat( weather_str, temperature[weather_no] );
        strcat( weather_str, temp_units );
        // APP_LOG( APP_LOG_LEVEL_DEBUG, "forecast: %s", weather_str );
        text_layer_set_text( weather_layer, weather_str );
    }
    if ( icon[weather_no] != NULL )
    {
        char c = icon[weather_no][0];
        int icon_no = c - '0';
        // APP_LOG( APP_LOG_LEVEL_DEBUG, "icon no. %i", icon_no );
        bitmap_layer_set_bitmap( weather_icon_layer, weather_bitmap[icon_no] );
    }
    weather_no = weather_no + 1;
    if ( weather_no == NO_FORECASTS ) weather_no = 0;
}

void in_received_handler( DictionaryIterator *iter, void *context ) 
{
    Tuple *clock_format_t = dict_find( iter, CLOCK_FORMAT );
    if ( clock_format_t )
    {
        strcpy( clock_format, clock_format_t->value->cstring );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "clock format: %s", clock_format );
    }
    Tuple *lon_t = dict_find( iter, LON );
    Tuple *lat_t = dict_find( iter, LAT );
    if ( lon_t && lat_t )
    {
        int len = strlen( lat_t->value->cstring ) + strlen( lon_t->value->cstring ) + 2;
        if ( geo_str != NULL )
        {
            free( geo_str );
        }
        geo_str = malloc( len );
        strcpy( geo_str, lon_t->value->cstring );
        strcat( geo_str, "," );
        strcat( geo_str, lat_t->value->cstring );
        // APP_LOG( APP_LOG_LEVEL_DEBUG, "geo: %s", geo_str );
    }
    else
    {
        Tuple *city_t = dict_find( iter, CITY );
        if ( city_t )
        {
            int len = strlen( city_t->value->cstring );
            if ( geo_str != NULL )
            {
                free( geo_str );
            }
            geo_str = malloc( len ) + 1;
            strcpy( geo_str, city_t->value->cstring );
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "geo: %s", geo_str );
        }

        Tuple *desc_t = dict_find( iter, DESCRIPTION );
        if ( desc_t )
        {
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "desc %s", desc_t->value->cstring );
            set_array( description, desc_t->value->cstring );
        }
        Tuple *datestamp_t = dict_find( iter, DATESTAMP );
        if ( datestamp_t )
        {
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "datestamp: %s", datestamp_t->value->cstring );
            set_array( datestamp, datestamp_t->value->cstring );
        }
        Tuple *temp_t = dict_find( iter, TEMP );
        Tuple *temp_units_t = dict_find( iter, TEMP_UNITS );
        if ( temp_t && temp_units_t )
        {
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "temp units: %s", temp_units_t->value->cstring );
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "temp: %s", temp_t->value->cstring );
            if ( strcmp( temp_units_t->value->cstring, "metric" ) == 0 )
            {
                strcpy( temp_units, " C" );
            }
            else
            {
                strcpy( temp_units, " F" );
            }
            set_array( temperature, temp_t->value->cstring );
        }
        Tuple *icon_t = dict_find( iter, ICON );
        if ( icon_t )
        {
            // APP_LOG( APP_LOG_LEVEL_DEBUG, "icon %s", icon_t->value->cstring );
            set_array( icon, icon_t->value->cstring );
        }
        weather_no = 0;
        cycle_weather();
    }
}

void in_dropped_handler( AppMessageResult reason, void *context ) 
{
    APP_LOG( APP_LOG_LEVEL_WARNING, "in_dropped_handler: reason = %d", reason );
}

static void handle_battery( BatteryChargeState charge_state ) 
{
    static char battery_text[] = "100% charging";

    APP_LOG( APP_LOG_LEVEL_DEBUG, "charge status: %d%%", charge_state.charge_percent );
    snprintf( battery_text, sizeof( battery_text ), "%d%%", charge_state.charge_percent );
    battery_level = persist_exists( BATTERY_LEVEL ) ? persist_read_int( BATTERY_LEVEL ) : 100;
    if ( battery_level >= 60 && charge_state.charge_percent < 60 )
    {
        vibes_short_pulse();
        persist_write_int( BATTERY_LEVEL, charge_state.charge_percent );
    }
    uint32_t resource_id;
    if ( charge_state.is_charging )
    {
        resource_id = RESOURCE_ID_BATTERY_CHARGING;
    }
    else if ( charge_state.charge_percent < 50 )
    {
        resource_id = RESOURCE_ID_BATTERY_25;
    }
    else if ( charge_state.charge_percent < 75 )
    {
        resource_id = RESOURCE_ID_BATTERY_50;
    }
    else if ( charge_state.charge_percent < 100 )
    {
        resource_id = RESOURCE_ID_BATTERY_75;
    }
    else
    {
        resource_id = RESOURCE_ID_BATTERY_100;
    }
    if ( battery_bitmap ) 
    {
        gbitmap_destroy( battery_bitmap );
    }
    battery_bitmap = gbitmap_create_with_resource( resource_id );
    bitmap_layer_set_bitmap( battery_icon_layer, battery_bitmap );
    text_layer_set_text( battery_layer, battery_text );
}


static void send_weather_request()
{
    notify( "get weather" );
    DictionaryIterator *iter;
    app_message_outbox_begin( &iter );
    if ( iter == NULL ) 
    {
        APP_LOG( APP_LOG_LEVEL_WARNING, "null iter" );
        return;
    }

    Tuplet tuple = TupletInteger( FETCH_WEATHER, 1 );
    dict_write_tuplet( iter, &tuple );
    dict_write_end( iter );

    app_message_outbox_send();
}

static void handle_tap( AccelAxisType axis, int32_t direction )
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "tap" );
    send_weather_request();
}

static void handle_day_tick( struct tm* tick_time, TimeUnits units_changed )
{
    static char date_text[] = "Mon 31 Dec"; // Needs to be static because it's used by the system later.

    strftime( date_text, sizeof( date_text ), "%a %d %b", tick_time );
    text_layer_set_text( date_layer, date_text );
}

static void handle_hour_tick( struct tm* tick_time, TimeUnits units_changed )
{
    if ( tick_time->tm_hour == 0 ) handle_day_tick( tick_time, units_changed );
}

static void handle_minute_tick( struct tm* tick_time, TimeUnits units_changed ) 
{
    static char time_text[] = "00:00"; // Needs to be static because it's used by the system later.
    static char am_text[] = "  "; // Needs to be static because it's used by the system later.

    if ( strcmp( clock_format, "12h" ) == 0 )
    {
        strftime( time_text, sizeof( time_text ), "%I:%M", tick_time );
        strftime( am_text, sizeof( am_text ), "%p", tick_time );
        for( char *p = am_text; *p; ++p ) *p = *p >0x40 && *p < 0x5b ? *p | 0x60 : *p;
    }
    else
    {
        strftime( time_text, sizeof( time_text ), "%H:%M", tick_time );
        strcpy( am_text, "  " );
    }
    text_layer_set_text( time_layer, time_text );
    text_layer_set_text( am_layer, am_text );
    if ( tick_time->tm_min == 0 ) handle_hour_tick( tick_time, units_changed );
}

static void handle_second_tick( struct tm* tick_time, TimeUnits units_changed ) 
{
    static char secs_text[] = ":00"; // Needs to be static because it's used by the system later.

    strftime( secs_text, sizeof( secs_text ), ":%S", tick_time );
    text_layer_set_text( secs_layer, secs_text );
    if ( tick_time->tm_sec == 0 ) handle_minute_tick( tick_time, units_changed );
    if ( tick_time->tm_sec % cycle_secs == 0 )
    {
        cycle_weather();
    }
}

static void init_time()
{
    time_t t = time( NULL );
    struct tm *lt = localtime( &t );
    handle_second_tick( lt, SECOND_UNIT );
    handle_minute_tick( lt, SECOND_UNIT );
    handle_hour_tick( lt, SECOND_UNIT );
    handle_day_tick( lt, SECOND_UNIT );
}

static void handle_bluetooth( bool connected ) 
{
    text_layer_set_text( connection_layer, connected ? "connected" : "disconnected" );
    if ( ! connected )
    {
        vibes_short_pulse();
        notify( "diconnected" );
    }
}

static void window_load( Window *window ) 
{
    root_layer = window_get_root_layer( window );
    frame = layer_get_frame( root_layer );

    GFont custom_font_36 = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CONSOLA_MONO_36 ) );

    time_layer = text_layer_create( GRect( 0, 0, frame.size.w, 50 ) );
    text_layer_set_text_color( time_layer, GColorWhite );
    text_layer_set_background_color( time_layer, GColorClear );
    text_layer_set_font( time_layer, custom_font_36 );
    text_layer_set_text_alignment( time_layer, GTextAlignmentLeft );
    text_layer_set_text( time_layer, "00:00" );

    GFont custom_font_20 = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CONSOLA_MONO_20 ) );

    secs_layer = text_layer_create( GRect( 105, 0, frame.size.w-100, 40 ) );
    text_layer_set_text_color( secs_layer, GColorWhite );
    text_layer_set_background_color( secs_layer, GColorClear );
    text_layer_set_font( secs_layer, custom_font_20 );
    text_layer_set_text_alignment( secs_layer, GTextAlignmentLeft );
    text_layer_set_text( secs_layer, ":00" );

    am_layer = text_layer_create( GRect( 110, 20, frame.size.w-110, 40 ) );
    text_layer_set_text_color( am_layer, GColorWhite );
    text_layer_set_background_color( am_layer, GColorClear );
    text_layer_set_font( am_layer, custom_font_20 );
    text_layer_set_text_alignment( am_layer, GTextAlignmentLeft );
    text_layer_set_text( am_layer, "  " );

    notify_layer = text_layer_create( GRect( 10, 70, frame.size.w-30, 50 ) );
    text_layer_set_text_color( notify_layer, GColorBlack );
    text_layer_set_background_color( notify_layer, GColorWhite );
    text_layer_set_font( notify_layer, fonts_get_system_font( FONT_KEY_GOTHIC_24_BOLD ) );
    text_layer_set_text_alignment( notify_layer, GTextAlignmentCenter );
    layer_set_hidden( text_layer_get_layer( notify_layer ), true );
    text_layer_set_overflow_mode( notify_layer, GTextOverflowModeWordWrap );

    date_layer = text_layer_create( GRect( 0, 40, frame.size.w, 30 ) );
    text_layer_set_text_color( date_layer, GColorWhite );
    text_layer_set_background_color( date_layer, GColorClear );
    text_layer_set_font( date_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( date_layer, GTextAlignmentLeft );
    text_layer_set_overflow_mode( notify_layer, GTextOverflowModeTrailingEllipsis );

    weather_icon_layer = bitmap_layer_create( GRect( 0, 70, 50, 50 ) );

    weather_layer = text_layer_create( GRect( 70, 60, frame.size.w-70, 120 ) );
    text_layer_set_text_color( weather_layer, GColorWhite );
    text_layer_set_background_color( weather_layer, GColorClear );
    text_layer_set_font( weather_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18_BOLD ) );
    text_layer_set_text_alignment( weather_layer, GTextAlignmentLeft );
    text_layer_set_overflow_mode( weather_layer, GTextOverflowModeWordWrap );

    battery_icon_layer = bitmap_layer_create( GRect( 0, 125, 16, 16 ) );
    battery_layer = text_layer_create( GRect( 20, 120, 60, 20 ) );
    text_layer_set_text_color( battery_layer, GColorWhite );
    text_layer_set_background_color( battery_layer, GColorClear );
    text_layer_set_font( battery_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( battery_layer, GTextAlignmentLeft );
    text_layer_set_text( battery_layer, "100%" );

    connection_layer = text_layer_create( GRect( 0, 140, 80, 20 ) );
    text_layer_set_text_color( connection_layer, GColorWhite );
    text_layer_set_background_color( connection_layer, GColorClear );
    text_layer_set_font( connection_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( connection_layer, GTextAlignmentLeft );
    text_layer_set_text( connection_layer, "disconnected" );

    layer_add_child( root_layer, text_layer_get_layer( time_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( am_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( date_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( secs_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( weather_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( connection_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( battery_layer ) );
    layer_add_child( root_layer, bitmap_layer_get_layer( weather_icon_layer ) );
    layer_add_child( root_layer, bitmap_layer_get_layer( battery_icon_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( notify_layer ) );

    init_time();
    tick_timer_service_subscribe( SECOND_UNIT, &handle_second_tick );
    handle_battery( battery_state_service_peek() );
    battery_state_service_subscribe( &handle_battery );
    handle_bluetooth( bluetooth_connection_service_peek() );
    bluetooth_connection_service_subscribe( &handle_bluetooth );

    accel_tap_service_subscribe( &handle_tap );
    app_message_register_inbox_received( in_received_handler );
    app_message_register_inbox_dropped( in_dropped_handler );
    app_message_register_outbox_sent( out_sent_handler );
    app_message_register_outbox_failed( out_failed_handler );

    app_message_open( inbound_size, outbound_size );
}

static void window_unload( Window *window ) 
{
    // app_sync_deinit( &sync );
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    text_layer_destroy( time_layer );
    text_layer_destroy( am_layer );
    text_layer_destroy( secs_layer );
    text_layer_destroy( date_layer );
    text_layer_destroy( connection_layer );
    text_layer_destroy( battery_layer );
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG( APP_LOG_LEVEL_DEBUG, "click" );
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG( APP_LOG_LEVEL_DEBUG, "long click" );
}

static void click_config_provider(void *context) {
    window_single_click_subscribe( BUTTON_ID_SELECT, select_click_handler );
    window_long_click_subscribe( BUTTON_ID_SELECT, 0, select_long_click_handler, NULL );
}

static void do_init( void ) 
{
    
    weather_bitmap[0] = gbitmap_create_with_resource( RESOURCE_ID_01d );
    
    weather_bitmap[1] = gbitmap_create_with_resource( RESOURCE_ID_02d );
    
    weather_bitmap[2] = gbitmap_create_with_resource( RESOURCE_ID_03d );
    
    weather_bitmap[3] = gbitmap_create_with_resource( RESOURCE_ID_04d );
    
    weather_bitmap[4] = gbitmap_create_with_resource( RESOURCE_ID_09d );
    
    weather_bitmap[5] = gbitmap_create_with_resource( RESOURCE_ID_10d );
    
    weather_bitmap[6] = gbitmap_create_with_resource( RESOURCE_ID_11d );
    
    weather_bitmap[7] = gbitmap_create_with_resource( RESOURCE_ID_13d );
    
    weather_bitmap[8] = gbitmap_create_with_resource( RESOURCE_ID_50d );
    
    weather_bitmap[9] = gbitmap_create_with_resource( RESOURCE_ID_01n );
    
    weather_bitmap[10] = gbitmap_create_with_resource( RESOURCE_ID_02n );
    
    weather_bitmap[11] = gbitmap_create_with_resource( RESOURCE_ID_03n );
    
    weather_bitmap[12] = gbitmap_create_with_resource( RESOURCE_ID_04n );
    
    weather_bitmap[13] = gbitmap_create_with_resource( RESOURCE_ID_09n );
    
    weather_bitmap[14] = gbitmap_create_with_resource( RESOURCE_ID_10n );
    
    weather_bitmap[15] = gbitmap_create_with_resource( RESOURCE_ID_11n );
    
    weather_bitmap[16] = gbitmap_create_with_resource( RESOURCE_ID_13n );
    
    weather_bitmap[17] = gbitmap_create_with_resource( RESOURCE_ID_50n );
    
    window = window_create();
    window_set_background_color( window, GColorBlack );
    window_set_fullscreen( window, true );
    window_set_window_handlers(
        window, 
        ( WindowHandlers ) {
            .load = window_load,
            .unload = window_unload
        }
    );
    window_set_click_config_provider( window, click_config_provider );
    window_stack_push( window, true );

}

static void do_deinit( void ) 
{
    window_destroy(window);
}

int main( void ) 
{
    do_init();
    app_event_loop();
    do_deinit();
}
