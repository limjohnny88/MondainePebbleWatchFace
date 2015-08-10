#include "pebble.h"
#include "main.h"

// Number of updates per second, must be < 1000
#define STOP2GO_TICK_RESOLUTION 4

/*
// Force the users to shake X times to dismiss the bluetooth disconnection screen
#define BLUETOOTH_DISCONNECTION_SHAKE_TIMES 2
static uint8_t s_bluetooth_disconnection_dismissal_attempt = 0;
*/

#define CONFIGS_KEY 0

static int s_persist_value_read, s_persist_value_written;
static AppSync s_sync;
static uint8_t s_sync_buffer[128];

typedef struct Persist {
  char dialcolor[10];
  char secondhandoption[10];
  char dateoption[10];
  char hourlyvibration[10];
  uint8_t bluetoothstatusdetection;
} __attribute__((__packed__)) Persist;

Persist configs = {
  .dialcolor = "white",
  .secondhandoption = "quartz",
  .dateoption = "nodate",
  .hourlyvibration = "off",
  .bluetoothstatusdetection = 0
};

enum {
  DialColor_KEY = 0x0,
  SecondHandOption_KEY = 0x1,
  DateOption_KEY = 0x2,
  HourlyVibration_KEY = 0x3,
  BluetoothStatusDetection_KEY = 0x4
};

static Window *s_window;
static Layer *s_hands_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static GPath *s_minute_arrow, *s_hour_arrow;

static GFont s_res_gothic_18_bold;
static TextLayer *s_textlayer_date;

Window *bluetooth_connected_splash_window;
static BitmapLayer *s_bluetoothconnected_layer;
static GBitmap *s_bluetoothconnected_bitmap;

Window *bluetooth_disconnected_splash_window;
static BitmapLayer *s_bluetoothdisconnected_layer;
static GBitmap *s_bluetoothdisconnected_bitmap;

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // Date
  if (strcmp(configs.dateoption, "nodate") == 0) {
    layer_set_hidden((Layer *)s_textlayer_date, true);
  }
  else {
    static char date[3];
    strftime(date, 3, "%d", t);
    text_layer_set_text(s_textlayer_date, date);
    
    #ifdef PBL_COLOR
      graphics_context_set_stroke_width(ctx, 2);
      if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorDarkGray);
        graphics_context_set_stroke_color(ctx, GColorDarkGray);
        text_layer_set_text_color(s_textlayer_date, GColorDarkGray);
      }
      else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        text_layer_set_text_color(s_textlayer_date, GColorWhite);
      };
    #else
      if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        text_layer_set_text_color(s_textlayer_date, GColorBlack);
      }
      else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        text_layer_set_text_color(s_textlayer_date, GColorWhite);
      }
    #endif
    
    // Date box
    graphics_draw_rect(ctx, GRect(102, 74, 22, 20));
    layer_set_hidden((Layer *)s_textlayer_date, false);
  }
 
  #ifdef PBL_COLOR
      graphics_context_set_stroke_width(ctx, 1);
  #endif
  
  // Hour hand
  if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
  }
  else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  // Minute/hour hand
  if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
  }
  else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }
  
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);
  
  // Second hand
  if (strcmp(configs.secondhandoption, "quartz") == 0 || strcmp(configs.secondhandoption, "stop2go") == 0) {
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorRed);
      graphics_context_set_stroke_color(ctx, GColorRed);
    #else
      if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorBlack);
      }
      else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
      }
    #endif
      
    #ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 3);
    #endif
      
    int16_t second_hand_length = (bounds.size.w / 2) - 22;
    int16_t second_hand_opp_length = 16;
     
    double second_angle = 0;
    
    if (strcmp(configs.secondhandoption, "quartz") == 0) {
      second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
    }
    // Stop2go
    else if (strcmp(configs.secondhandoption, "stop2go") == 0) {
      // Move the second hand around the watch in 58 seconds (60/58 = 1.03448275862)
      second_angle = TRIG_MAX_ANGLE * 1.03448275862 * (t->tm_sec / 60.0 + time_ms(NULL, NULL) / 60000.0);
      // Pause the second at 12 o'clock mark
      second_angle = (second_angle >= TRIG_MAX_ANGLE) ? TRIG_MAX_ANGLE : second_angle;
    }
    
    GPoint second_hand = {
      .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x ,
      .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y ,
    };
    
    graphics_draw_line(ctx, second_hand, center);
    
    GPoint second_hand_opp = {
      .x = (int16_t)(-sin_lookup(second_angle) * (int32_t)second_hand_opp_length / TRIG_MAX_RATIO) + center.x ,
      .y = (int16_t)(cos_lookup(second_angle) * (int32_t)second_hand_opp_length / TRIG_MAX_RATIO) + center.y ,
    };
    
    graphics_draw_line(ctx, second_hand_opp, center);
    
     // Second hand circle
    graphics_fill_circle(ctx, second_hand, 5);
    
    
    // Dot in the middle
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorRed);
      graphics_context_set_stroke_color(ctx, GColorRed);
    #else
      if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorBlack);
      }
      else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
      }
    #endif
    
    graphics_fill_circle(ctx, GPoint(bounds.size.w / 2,bounds.size.h / 2), 4);
  }
}

static void load_background_image() {
  gbitmap_destroy(s_background_bitmap);
  if (strcmp(configs.dialcolor, "white") == 0) {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MONDAINE_WHITEBG);
  }
  else if (strcmp(configs.dialcolor, "white_nl") == 0) {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MONDAINE_WHITENLBG);
  }
  else if (strcmp(configs.dialcolor, "black") == 0) {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MONDAINE_BLACKBG);
  }
   else if (strcmp(configs.dialcolor, "black_nl") == 0) {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MONDAINE_BLACKNLBG);
  }
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
}


static void handle_tick(struct tm *t, TimeUnits units_changed) {
  if (strcmp(configs.secondhandoption, "stop2go") == 0) {
    for (int i=0; i<STOP2GO_TICK_RESOLUTION; i++) {
  		app_timer_register(1000 / STOP2GO_TICK_RESOLUTION * i, (void*)layer_mark_dirty, window_get_root_layer(s_window));
    }
  }
  else {
    layer_mark_dirty(window_get_root_layer(s_window));
  }
  
    
  // Hourly vibration
  if (t->tm_min == 0 && t->tm_sec == 0) {
    if (strcmp(configs.hourlyvibration, "short") == 0) {
      vibes_short_pulse();
    }
    else if (strcmp(configs.hourlyvibration, "long") == 0) {
      vibes_long_pulse();
    }
    else if (strcmp(configs.hourlyvibration, "double") == 0) {
      vibes_double_pulse();
    }
  }

}

static void load_persistent_config() {
  if (persist_exists(CONFIGS_KEY)) {
    s_persist_value_read = persist_read_data(CONFIGS_KEY, &configs, sizeof(configs));
  }
}

static void save_persistent_config() {
  s_persist_value_written = persist_write_data(CONFIGS_KEY, &configs, sizeof(configs));
}


static void s_sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_tuple_changed_callback, key: %lu", key);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_tuple_changed_callback, new_tuple->value->cstring: %s", new_tuple->value->cstring);
  
  switch (key) {
    case DialColor_KEY:  
      strcpy(configs.dialcolor, new_tuple->value->cstring);
    
      load_background_image();
    
      break;
    case SecondHandOption_KEY:
      strcpy(configs.secondhandoption, new_tuple->value->cstring);
 
      if (strcmp(configs.secondhandoption, "quartz") == 0) {
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else if (strcmp(configs.secondhandoption, "stop2go") == 0) {
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else if (strcmp(configs.secondhandoption, "off") == 0) {
        tick_timer_service_unsubscribe();
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
      }
    
      break;
    case DateOption_KEY:
      strcpy(configs.dateoption, new_tuple->value->cstring);
    
      break;
    case HourlyVibration_KEY:
      strcpy(configs.hourlyvibration, new_tuple->value->cstring);
    
      break;
    case BluetoothStatusDetection_KEY:
      configs.bluetoothstatusdetection = new_tuple->value->uint8;
    
      break;
  }
  

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_tick(t, HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);
}



static void bluetooth_connected_splash_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  
  // Load bluetooth connected image
  s_bluetoothconnected_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  s_bluetoothconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHCONNECTED);
  bitmap_layer_set_bitmap(s_bluetoothconnected_layer, s_bluetoothconnected_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bluetoothconnected_layer));
}

static void bluetooth_connected_window_unload(Window *window)
{
  gbitmap_destroy(s_bluetoothconnected_bitmap);
  bitmap_layer_destroy(s_bluetoothconnected_layer);
}
      
static void bluetooth_disconnected_splash_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  
  // Load bluetooth disconnected image
  s_bluetoothdisconnected_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  s_bluetoothdisconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHDISCONNECTED);
  bitmap_layer_set_bitmap(s_bluetoothdisconnected_layer, s_bluetoothdisconnected_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bluetoothdisconnected_layer));
}

static void bluetooth_disconnected_window_unload(Window *window)
{
  gbitmap_destroy(s_bluetoothdisconnected_bitmap);
  bitmap_layer_destroy(s_bluetoothdisconnected_layer);
}

static void hide_bluetooth_connected_splash_window() {
  window_stack_remove(bluetooth_connected_splash_window, true);
}

static void hide_bluetooth_disconnected_splash_window() {
  window_stack_remove(bluetooth_disconnected_splash_window, true);
}

static void handle_bluetooth_connected () {
  window_stack_remove(bluetooth_disconnected_splash_window, true);
  window_stack_push(bluetooth_connected_splash_window, true);

  vibes_long_pulse();
  // Hide splash screen
  app_timer_register(2000, (void*)hide_bluetooth_connected_splash_window, NULL);
  light_enable_interaction();
  //s_bluetooth_disconnection_dismissal_attempt = 0;
}

static void handle_bluetooth_disconnected () {
  window_stack_remove(bluetooth_connected_splash_window, true);
  window_stack_push(bluetooth_disconnected_splash_window, true);
      
  uint32_t vibes_pluse_segments[] = { 200, 100, 200, 100, 200, 100, 200, 100, 200 };
  VibePattern vibes_pluse_pattern = {
    .durations = vibes_pluse_segments,
    .num_segments = ARRAY_LENGTH(vibes_pluse_segments),
  };

  vibes_enqueue_custom_pattern(vibes_pluse_pattern);
  light_enable_interaction();
  // Hide splash screen
  app_timer_register(5000, (void*)hide_bluetooth_disconnected_splash_window, NULL);
}

static void bluetooth_connection_callback(bool is_connected) {
  if (configs.bluetoothstatusdetection == 1) {
    if (is_connected) {
      handle_bluetooth_connected();
    }
    else {
      handle_bluetooth_disconnected();
    }
  }
}

static void startup_bluetooth_disconnection_callback(bool is_connected) {
  if (configs.bluetoothstatusdetection == 1) {
    if (!is_connected) {
      handle_bluetooth_disconnected();
    }
  }
}

/*
static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (configs.bluetoothstatusdetection == 1 && !bluetooth_connection_service_peek()) {
    s_bluetooth_disconnection_dismissal_attempt++;
    if (s_bluetooth_disconnection_dismissal_attempt >= BLUETOOTH_DISCONNECTION_SHAKE_TIMES) {
      window_stack_remove(bluetooth_disconnected_splash_window, true);
      s_bluetooth_disconnection_dismissal_attempt = 0;
    }
  }
}
*/

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Load Mondaine background image
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  load_background_image();
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  // Initalize date layer
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  static char date[3];
  strftime(date, 3, "%d", t);
  
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  s_textlayer_date = text_layer_create(GRect(102, 72, 22, 20));

  #ifdef PBL_COLOR
    if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
      text_layer_set_text_color(s_textlayer_date, GColorDarkGray);
    }
    else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
      text_layer_set_text_color(s_textlayer_date, GColorWhite);
    };
  #else
    if (strcmp(configs.dialcolor, "white") == 0 || strcmp(configs.dialcolor, "white_nl") == 0) {
      text_layer_set_text_color(s_textlayer_date, GColorBlack);
    }
    else if (strcmp(configs.dialcolor, "black") == 0 || strcmp(configs.dialcolor, "black_nl") == 0) {
      text_layer_set_text_color(s_textlayer_date, GColorWhite);
    }
  #endif
    
  text_layer_set_background_color(s_textlayer_date, GColorClear);
  text_layer_set_text(s_textlayer_date, date);
  text_layer_set_text_alignment(s_textlayer_date, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_date, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_date);
  
  if (strcmp(configs.dateoption, "nodate") == 0) {
    layer_set_hidden((Layer *)s_textlayer_date, true);
  }
  
  // Initalize hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);

  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);
  
  layer_destroy(s_hands_layer);
  text_layer_destroy(s_textlayer_date);
}

static void init() {
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);
  
  load_persistent_config();
  
  
  Tuplet initial_values[] = {
    TupletCString(DialColor_KEY, configs.dialcolor),
    TupletCString(SecondHandOption_KEY, configs.secondhandoption),
    TupletCString(DateOption_KEY, configs.dateoption),
    TupletCString(HourlyVibration_KEY, configs.hourlyvibration),
    TupletInteger(BluetoothStatusDetection_KEY, configs.bluetoothstatusdetection)
  };
  
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), s_sync_tuple_changed_callback, NULL, NULL);
  
  
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

 
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);
  
  if (strcmp(configs.secondhandoption, "off") == 0) {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  }
  else if (strcmp(configs.secondhandoption, "quartz") == 0){
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  }
  else if (strcmp(configs.secondhandoption, "stop2go") == 0){
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  }
  else {
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  }
  
  // Bluetooth status detection
  bluetooth_connected_splash_window = window_create();
  window_set_window_handlers(bluetooth_connected_splash_window, (WindowHandlers) {
   .load = bluetooth_connected_splash_window_load,
   .unload = bluetooth_connected_window_unload,
  });
  
  bluetooth_disconnected_splash_window = window_create();
  window_set_window_handlers(bluetooth_disconnected_splash_window, (WindowHandlers) {
   .load = bluetooth_disconnected_splash_window_load,
   .unload = bluetooth_disconnected_window_unload,
  });
  
  // Check bluetooth connection on page load, do action only when bluetooth is disconected.
  startup_bluetooth_disconnection_callback(bluetooth_connection_service_peek());
  
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  //accel_tap_service_subscribe(tap_handler); 
}

static void deinit() {
  save_persistent_config();
  
  //accel_tap_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  
  app_sync_deinit(&s_sync);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
