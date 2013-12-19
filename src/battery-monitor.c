// Standard includes
#include "pebble.h"


// App-specific data
Window *window; // All apps must have at least one window

TextLayer *time_layer; 
TextLayer *secs_layer; 
TextLayer *geo_layer; 
TextLayer *weather_layer; 
TextLayer *date_layer; 
TextLayer *battery_layer;
TextLayer *connection_layer;

const uint32_t inbound_size = 64;
const uint32_t outbound_size = 64;

#define BATTERY_LEVEL 1
static int battery_level = 100;

enum GeoKey {
    LAT = 0x0,
    LON = 0x1,
    TEMP = 0x2,
    CITY = 0x3,
    DESCRIPTION = 0x4,
    TEMP_UNITS = 0x5,
};

void out_sent_handler( DictionaryIterator *sent, void *context ) 
{
}


void out_failed_handler( DictionaryIterator *failed, AppMessageResult reason, void *context ) 
{
}


void in_received_handler( DictionaryIterator *iter, void *context ) 
{
    Tuple *lon_t = dict_find( iter, LON );
    Tuple *lat_t = dict_find( iter, LAT );
    if ( lon_t && lat_t )
    {
        static char geo_text[] = "                                         ";
        strcpy( geo_text, "" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "in_received_handler" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "lon" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, lon_t->value->cstring );
        strcat( geo_text, lon_t->value->cstring );
        strcat( geo_text, ", " );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "lat" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, lat_t->value->cstring );
        strcat( geo_text, lat_t->value->cstring );
        text_layer_set_text( geo_layer, geo_text );
    }

    Tuple *city = dict_find( iter, CITY );
    if ( city )
    {
        static char geo_text[] = "                                         ";
        strcpy( geo_text, "" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "in_received_handler" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "city" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, city->value->cstring );
        strcat( geo_text, city->value->cstring );
        text_layer_set_text( geo_layer, geo_text );
    }

    Tuple *desc_t = dict_find( iter, DESCRIPTION );
    Tuple *temp_t = dict_find( iter, TEMP );
    Tuple *temp_units_t = dict_find( iter, TEMP_UNITS );
    if ( temp_t && desc_t )
    {
        static char weather_text[] = "                                       ";
        strcpy( weather_text, "" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "description" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, desc_t->value->cstring );
        strcat( weather_text, desc_t->value->cstring );
        strcat( weather_text, ", " );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "temp" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, temp_t->value->cstring );
        strcat( weather_text, temp_t->value->cstring );
        APP_LOG( APP_LOG_LEVEL_DEBUG, "temp_units" );
        APP_LOG( APP_LOG_LEVEL_DEBUG, temp_units_t->value->cstring );
        if ( strcmp( temp_units_t->value->cstring, "metric" ) == 0 )
        {
            strcat( weather_text, "°C" );
        }
        else
        {
            strcat( weather_text, "°F" );
        }
        text_layer_set_text( weather_layer, weather_text );
    }
}


void in_dropped_handler( AppMessageResult reason, void *context ) 
{
}

static void handle_battery( BatteryChargeState charge_state ) 
{
    static char battery_text[] = "battery: 100% charging";

    APP_LOG( APP_LOG_LEVEL_DEBUG, "%d %%", charge_state.charge_percent );
    if ( charge_state.is_charging )
    {
        APP_LOG( APP_LOG_LEVEL_DEBUG, "charging" );
        snprintf( battery_text, sizeof( battery_text ), "battery: %d%% charging", charge_state.charge_percent );
    }
    else 
    {
        snprintf( battery_text, sizeof( battery_text ), "battery: %d%%", charge_state.charge_percent );
        battery_level = persist_exists( BATTERY_LEVEL ) ? persist_read_int( BATTERY_LEVEL ) : 100;
        if ( battery_level >= 60 && charge_state.charge_percent < 60 )
        {
            APP_LOG( APP_LOG_LEVEL_DEBUG, "battery low" );
            vibes_short_pulse();
            persist_write_int( BATTERY_LEVEL, charge_state.charge_percent );
        }
    }
    text_layer_set_text( battery_layer, battery_text );
}

// Called once per second
static void handle_second_tick( struct tm* tick_time, TimeUnits units_changed ) 
{
    static char time_text[] = "00:00"; // Needs to be static because it's used by the system later.
    static char secs_text[] = "00"; // Needs to be static because it's used by the system later.
    static char date_text[] = "Mon 31 Dec"; // Needs to be static because it's used by the system later.

    strftime( date_text, sizeof( date_text ), "%a %d %b", tick_time );
    text_layer_set_text( date_layer, date_text );
    strftime( time_text, sizeof( time_text ), "%H:%M", tick_time );
    text_layer_set_text( time_layer, time_text );
    strftime( secs_text, sizeof( secs_text ), "%S", tick_time );
    text_layer_set_text( secs_layer, secs_text );
}

static void handle_bluetooth( bool connected ) 
{
    text_layer_set_text( connection_layer, connected ? "bluetooth: connected" : "bluetooth: disconnected" );
    if ( ! connected )
    {
        APP_LOG( APP_LOG_LEVEL_DEBUG, "disconnected" );
        vibes_short_pulse();
    }
}

static void window_load( Window *window ) 
{
    APP_LOG( APP_LOG_LEVEL_DEBUG, "window_load" );
    Layer *root_layer;
    root_layer = window_get_root_layer( window );
    GRect frame = layer_get_frame( root_layer );

    time_layer = text_layer_create( GRect( 0, 0, frame.size.w /* width */, 50/* height */ ) );
    text_layer_set_text_color( time_layer, GColorWhite );
    text_layer_set_background_color( time_layer, GColorClear );
    text_layer_set_font( time_layer, fonts_get_system_font( FONT_KEY_ROBOTO_BOLD_SUBSET_49 ) );
    text_layer_set_text_alignment( time_layer, GTextAlignmentLeft );
    text_layer_set_text( time_layer, "00:00" );

    date_layer = text_layer_create( GRect( 0, 60, frame.size.w /* width */, 30/* height */ ) );
    text_layer_set_text_color( date_layer, GColorWhite );
    text_layer_set_background_color( date_layer, GColorClear );
    text_layer_set_font( date_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( date_layer, GTextAlignmentLeft );
    text_layer_set_text( date_layer, "" );

    secs_layer = text_layer_create( GRect( 80, 50, frame.size.w /* width */, 40/* height */ ) );
    text_layer_set_text_color( secs_layer, GColorWhite );
    text_layer_set_background_color( secs_layer, GColorClear );
    text_layer_set_font( secs_layer, fonts_get_system_font( FONT_KEY_BITHAM_34_MEDIUM_NUMBERS ) );
    text_layer_set_text_alignment( secs_layer, GTextAlignmentLeft );
    text_layer_set_text( secs_layer, "00" );

    geo_layer = text_layer_create( GRect( 0, 80, frame.size.w /* width */, 20/* height */ ) );
    text_layer_set_text_color( geo_layer, GColorWhite );
    text_layer_set_background_color( geo_layer, GColorClear );
    text_layer_set_font( geo_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( geo_layer, GTextAlignmentLeft );
    text_layer_set_text( geo_layer, "" );

    weather_layer = text_layer_create( GRect( 0, 100, frame.size.w /* width */, 20/* height */ ) );
    text_layer_set_text_color( weather_layer, GColorWhite );
    text_layer_set_background_color( weather_layer, GColorClear );
    text_layer_set_font( weather_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( weather_layer, GTextAlignmentLeft );
    text_layer_set_text( weather_layer, "" );

    connection_layer = text_layer_create( GRect( 0, 120, /* width */ frame.size.w, 20 /* height */ ) );
    text_layer_set_text_color( connection_layer, GColorWhite );
    text_layer_set_background_color( connection_layer, GColorClear );
    text_layer_set_font( connection_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( connection_layer, GTextAlignmentLeft );
    text_layer_set_text( connection_layer, "bluetooth: disconnected" );

    battery_layer = text_layer_create( GRect( 0, 140, /* width */ frame.size.w, 20 /* height */ ) );
    text_layer_set_text_color( battery_layer, GColorWhite );
    text_layer_set_background_color( battery_layer, GColorClear );
    text_layer_set_font( battery_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
    text_layer_set_text_alignment( battery_layer, GTextAlignmentLeft );
    text_layer_set_text( battery_layer, "battery: 100% charged" );

    layer_add_child( root_layer, text_layer_get_layer( time_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( date_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( secs_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( geo_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( weather_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( connection_layer ) );
    layer_add_child( root_layer, text_layer_get_layer( battery_layer ) );

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
