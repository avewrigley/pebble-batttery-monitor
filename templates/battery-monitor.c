// Standard includes
#include "pebble.h"


// App-specific data
Window *window; // All apps must have at least one window

Layer *root_layer;
GRect frame;

TextLayer *time_layer; 
TextLayer *am_layer; 
TextLayer *secs_layer; 
TextLayer *geo_layer; 
TextLayer *weather_layer; 
TextLayer *temp_layer; 
TextLayer *date_layer; 
TextLayer *battery_layer;
TextLayer *connection_layer;
TextLayer *notify_layer; 

static GBitmap *weather_bitmap = NULL;
static BitmapLayer *weather_icon_layer;

static GBitmap *battery_bitmap = NULL;
static BitmapLayer *battery_icon_layer;

const uint32_t inbound_size = 256;
const uint32_t outbound_size = 256;

#define BATTERY_LEVEL 1
static int battery_level = 100;
static char clock_format[] = "12h";

static uint32_t WEATHER_ICONS[] = {
[% FOREACH image IN images %]
    RESOURCE_ID_[% image.name %][% UNLESS loop.last %],[% END %][% END %]
};

enum GeoKey {
    LAT = 0x0,
    LON = 0x1,
    TEMP = 0x2,
    CITY = 0x3,
    DESCRIPTION = 0x4,
    TEMP_UNITS = 0x5,
    CLOCK_FORMAT = 0x6,
    ICON = 0x7
};

static AppTimer *timer;

static void notify_timer_callback( void *data ) {
    layer_set_hidden( text_layer_get_layer( notify_layer ), true );
}

static void notify( char *text )
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "notify" );
    APP_LOG( APP_LOG_LEVEL_DEBUG, text );
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


void in_received_handler( DictionaryIterator *iter, void *context ) 
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "in_received_handler" );
    Tuple *lon_t = dict_find( iter, LON );
    Tuple *lat_t = dict_find( iter, LAT );
    Tuple *clock_format_t = dict_find( iter, CLOCK_FORMAT );
    if ( lon_t && lat_t && clock_format_t )
    {
        notify( "get weather" );
        static char geo_text[] = "                                         ";
        strcpy( geo_text, "" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "lon" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, lon_t->value->cstring );
        strcat( geo_text, lon_t->value->cstring );
        strcat( geo_text, ", " );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "lat" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, lat_t->value->cstring );
        strcat( geo_text, lat_t->value->cstring );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "geo_text" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, geo_text );
        text_layer_set_text( geo_layer, geo_text );
        strcpy( clock_format, clock_format_t->value->cstring );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "clock_format" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, clock_format );
    }
    else
    {
        Tuple *city_t = dict_find( iter, CITY );
        if ( city_t )
        {
            APP_LOG( APP_LOG_LEVEL_DEBUG, "city" );
            APP_LOG( APP_LOG_LEVEL_DEBUG, city_t->value->cstring );
            text_layer_set_text( geo_layer, city_t->value->cstring );
        }

        Tuple *desc_t = dict_find( iter, DESCRIPTION );
        Tuple *temp_t = dict_find( iter, TEMP );
        Tuple *temp_units_t = dict_find( iter, TEMP_UNITS );
        Tuple *icon_t = dict_find( iter, ICON );
        if ( temp_t && desc_t && temp_units_t )
        {
            APP_LOG( APP_LOG_LEVEL_DEBUG, "description" );
            APP_LOG( APP_LOG_LEVEL_DEBUG, desc_t->value->cstring );
            text_layer_set_text( weather_layer, desc_t->value->cstring );
            static char temp_text[] = "                                       ";
            APP_LOG( APP_LOG_LEVEL_DEBUG, "temp" );
            APP_LOG( APP_LOG_LEVEL_DEBUG, temp_t->value->cstring );
            strcpy( temp_text, temp_t->value->cstring );
            APP_LOG( APP_LOG_LEVEL_DEBUG, "temp_units" );
            APP_LOG( APP_LOG_LEVEL_DEBUG, temp_units_t->value->cstring );
            if ( strcmp( temp_units_t->value->cstring, "metric" ) == 0 )
            {
                strcat( temp_text, " C" );
            }
            else
            {
                strcat( temp_text, " F" );
            }
            APP_LOG( APP_LOG_LEVEL_DEBUG, temp_text );
            text_layer_set_text( temp_layer, temp_text );
            if ( icon_t )
            {
                if ( weather_bitmap ) 
                {
                    gbitmap_destroy( weather_bitmap );
                }
                APP_LOG( APP_LOG_LEVEL_DEBUG, "icon" );
                APP_LOG( APP_LOG_LEVEL_DEBUG, "%d", ( int ) icon_t->value->uint8 );
                uint32_t resource_id = WEATHER_ICONS[icon_t->value->uint8];
                APP_LOG( APP_LOG_LEVEL_DEBUG, "%d", ( int ) resource_id );
                weather_bitmap = gbitmap_create_with_resource( resource_id );
                bitmap_layer_set_bitmap( weather_icon_layer, weather_bitmap );
            }
        }
    }
}

void in_dropped_handler( AppMessageResult reason, void *context ) 
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "in_dropped_handler" );
    APP_LOG( APP_LOG_LEVEL_DEBUG, "%d", reason );
}

static void handle_battery( BatteryChargeState charge_state ) 
{
    static char battery_text[] = "100% charging";

    APP_LOG( APP_LOG_LEVEL_DEBUG, "%d %%", charge_state.charge_percent );
    snprintf( battery_text, sizeof( battery_text ), "%d%%", charge_state.charge_percent );
    battery_level = persist_exists( BATTERY_LEVEL ) ? persist_read_int( BATTERY_LEVEL ) : 100;
    if ( battery_level >= 60 && charge_state.charge_percent < 60 )
    {
        APP_LOG( APP_LOG_LEVEL_DEBUG, "battery low" );
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
    APP_LOG( APP_LOG_LEVEL_DEBUG, "resource_id: %d", ( int ) resource_id );
    if ( battery_bitmap ) 
    {
        gbitmap_destroy( battery_bitmap );
    }
    battery_bitmap = gbitmap_create_with_resource( resource_id );
    bitmap_layer_set_bitmap( battery_icon_layer, battery_bitmap );
    text_layer_set_text( battery_layer, battery_text );
}


// Called once per second
static void handle_second_tick( struct tm* tick_time, TimeUnits units_changed ) 
{
    static char time_text[] = "00:00"; // Needs to be static because it's used by the system later.
    static char am_text[] = "  "; // Needs to be static because it's used by the system later.
    static char secs_text[] = ":00"; // Needs to be static because it's used by the system later.
    static char date_text[] = "Mon 31 Dec"; // Needs to be static because it's used by the system later.

    strftime( date_text, sizeof( date_text ), "%a %d %b", tick_time );
    text_layer_set_text( date_layer, date_text );
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
    strftime( secs_text, sizeof( secs_text ), ":%S", tick_time );
    text_layer_set_text( secs_layer, secs_text );
}

static void handle_bluetooth( bool connected ) 
{
    text_layer_set_text( connection_layer, connected ? "connected" : "disconnected" );
    if ( ! connected )
    {
        APP_LOG( APP_LOG_LEVEL_DEBUG, "disconnected" );
        vibes_short_pulse();
        notify( "diconnected" );
    }
}

static void window_load( Window *window ) 
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_load" );
    root_layer = window_get_root_layer( window );
    frame = layer_get_frame( root_layer );

    APP_LOG(APP_LOG_LEVEL_DEBUG, "WIDTH: %d", frame.size.w );
    APP_LOG(APP_LOG_LEVEL_DEBUG, "HEIGHT: %d", frame.size.h );

    time_layer = text_layer_create( GRect( 0, 0, frame.size.w, 50 ) );
    text_layer_set_text_color( time_layer, GColorWhite );
    text_layer_set_background_color( time_layer, GColorClear );
    text_layer_set_font( time_layer, fonts_get_system_font( FONT_KEY_BITHAM_34_MEDIUM_NUMBERS ) );
    text_layer_set_text_alignment( time_layer, GTextAlignmentLeft );
    text_layer_set_text( time_layer, "00:00" );

    secs_layer = text_layer_create( GRect( 110, 0, frame.size.w-110, 40 ) );
    text_layer_set_text_color( secs_layer, GColorWhite );
    text_layer_set_background_color( secs_layer, GColorClear );
    text_layer_set_font( secs_layer, fonts_get_system_font( FONT_KEY_GOTHIC_28_BOLD ) );
    text_layer_set_text_alignment( secs_layer, GTextAlignmentLeft );
    text_layer_set_text( secs_layer, ":00" );

    am_layer = text_layer_create( GRect( 110, 20, frame.size.w-110, 40 ) );
    text_layer_set_text_color( am_layer, GColorWhite );
    text_layer_set_background_color( am_layer, GColorClear );
    text_layer_set_font( am_layer, fonts_get_system_font( FONT_KEY_GOTHIC_24 ) );
    text_layer_set_text_alignment( am_layer, GTextAlignmentLeft );
    text_layer_set_text( am_layer, "  " );

    notify_layer = text_layer_create( GRect( 5, 50, frame.size.w-35, 40 ) );
    text_layer_set_text_color( notify_layer, GColorBlack );
    text_layer_set_background_color( notify_layer, GColorWhite );
    text_layer_set_font( notify_layer, fonts_get_system_font( FONT_KEY_GOTHIC_24_BOLD ) );
    text_layer_set_text_alignment( notify_layer, GTextAlignmentCenter );
    layer_set_hidden( text_layer_get_layer( notify_layer ), true );
    text_layer_set_overflow_mode( notify_layer, GTextOverflowModeWordWrap );

    date_layer = text_layer_create( GRect( 0, 50, frame.size.w, 30 ) );
    text_layer_set_text_color( date_layer, GColorWhite );
    text_layer_set_background_color( date_layer, GColorClear );
    text_layer_set_font( date_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( date_layer, GTextAlignmentLeft );
    text_layer_set_text( date_layer, "" );

    geo_layer = text_layer_create( GRect( 70, 50, frame.size.w-70, 30 ) );
    text_layer_set_text_color( geo_layer, GColorWhite );
    text_layer_set_background_color( geo_layer, GColorClear );
    text_layer_set_font( geo_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( geo_layer, GTextAlignmentLeft );
    text_layer_set_text( geo_layer, "" );
    text_layer_set_overflow_mode( geo_layer, GTextOverflowModeWordWrap );

    weather_icon_layer = bitmap_layer_create( GRect( 0, 80, 50, 50 ) );

    weather_layer = text_layer_create( GRect( 70, 70, 74, 40 ) );
    text_layer_set_text_color( weather_layer, GColorWhite );
    text_layer_set_background_color( weather_layer, GColorClear );
    text_layer_set_font( weather_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( weather_layer, GTextAlignmentLeft );
    text_layer_set_text( weather_layer, "" );
    text_layer_set_overflow_mode( weather_layer, GTextOverflowModeWordWrap );

    temp_layer = text_layer_create( GRect( 70, 110, 74, 20 ) );
    text_layer_set_text_color( temp_layer, GColorWhite );
    text_layer_set_background_color( temp_layer, GColorClear );
    text_layer_set_font( temp_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( temp_layer, GTextAlignmentLeft );
    text_layer_set_text( temp_layer, "" );

    battery_icon_layer = bitmap_layer_create( GRect( 0, 145, 16, 16 ) );

    battery_layer = text_layer_create( GRect( 20, 140, 60, 20 ) );
    text_layer_set_text_color( battery_layer, GColorWhite );
    text_layer_set_background_color( battery_layer, GColorClear );
    text_layer_set_font( battery_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( battery_layer, GTextAlignmentLeft );
    text_layer_set_text( battery_layer, "100%" );

    connection_layer = text_layer_create( GRect( 70, 140, 80, 20 ) );
    text_layer_set_text_color( connection_layer, GColorWhite );
    text_layer_set_background_color( connection_layer, GColorClear );
    text_layer_set_font( connection_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( connection_layer, GTextAlignmentLeft );
    text_layer_set_text( connection_layer, "disconnected" );

    layer_add_child( root_layer, text_layer_get_layer( time_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( am_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( date_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( secs_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( geo_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( weather_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( temp_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( connection_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( battery_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( notify_layer ) );
    layer_add_child( root_layer, bitmap_layer_get_layer( weather_icon_layer ) );
    layer_add_child( root_layer, bitmap_layer_get_layer( battery_icon_layer ) );

    tick_timer_service_subscribe( SECOND_UNIT, &handle_second_tick );
    handle_battery( battery_state_service_peek() );
    battery_state_service_subscribe( &handle_battery );
    handle_bluetooth( bluetooth_connection_service_peek() );
    bluetooth_connection_service_subscribe( &handle_bluetooth );

    APP_LOG(APP_LOG_LEVEL_DEBUG, "register message handlers" );
    app_message_register_inbox_received( in_received_handler );
    app_message_register_inbox_dropped( in_dropped_handler );
    app_message_register_outbox_sent( out_sent_handler );
    app_message_register_outbox_failed( out_failed_handler );

    APP_LOG( APP_LOG_LEVEL_DEBUG, "app_message_open" );
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

// Handle the start-up of the app
static void do_init( void ) 
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "do_init");
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
    APP_LOG( APP_LOG_LEVEL_DEBUG, "do_deinit" );
    window_destroy(window);
}

int main( void ) 
{
    do_init();
    app_event_loop();
    do_deinit();
}
