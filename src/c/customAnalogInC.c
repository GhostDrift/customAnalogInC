#include "simple_analog.h"

#include "pebble.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer, *s_battery_layer;
static TextLayer *s_day_label, *s_num_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow, *s_date_box;
static char s_num_buffer[4], s_day_buffer[6];
//define variable to save the battery level to
static int s_battery_level;
static GColor s_battery_color;
static GPathInfo s_adjusted_bg_points[NUM_CLOCK_TICKS];
static GPathInfo s_adjusted_date_box_points[1];
static GPathInfo s_adjusted_minute_hand_points[1];
static GPathInfo s_adjusted_hour_hand_points[1];

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, s_tick_paths[i]);
    // gpath_draw_outline(ctx, s_tick_paths[i]);
  }
  //draw the date box
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_context_set_stroke_width(ctx, 2);
  gpath_draw_filled(ctx, s_date_box);
  gpath_draw_outline(ctx, s_date_box);
  graphics_context_set_stroke_width(ctx, 1);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int16_t second_hand_length = bounds.size.w / 2;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  // second hand
  graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
  graphics_draw_line(ctx, second_hand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // // dot in the middle
  // graphics_context_set_fill_color(ctx, GColorBlack);
  // graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
  // graphics_context_set_fill_color(ctx, GColorGreen);
  // graphics_fill_circle(ctx, GPoint(center.x , center.y), 3);

}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery: %d", s_battery_level);
  if (s_battery_level > 50){
    s_battery_color = GColorGreen;
  } else if ((s_battery_level <= 50) && (s_battery_level > 10)){
    s_battery_color = GColorYellow;
  } else if (s_battery_level <= 10) {
    s_battery_color = GColorRed;
  }

  graphics_context_set_fill_color(ctx, s_battery_color);
  graphics_fill_circle(ctx, GPoint(center.x , center.y), 3);


}

static void battery_callback(BatteryChargeState state){
  //Record the new battery level
  s_battery_level = state.charge_percent;
  
  //update meeter
  layer_mark_dirty(s_battery_layer);
}

//Helper function to scale and recenter GPathInfo Points for current screen size
static void scale_gpath_info(const GPathInfo *source, GPathInfo *dest, float scale_x, float scale_y, GPoint center){
  dest->num_points = source->num_points;
  dest->points = malloc(sizeof(GPoint) * dest->num_points);
  for (uint32_t i = 0; i < source->num_points; ++i) {
    int base_x = source->points[i].x;
    int base_y = source->points[i].y;

    //Calculate offset from base center (72, 84)
    int offset_x = base_x - 72;
    int offset_y = base_y - 84;

    //Scale and recenter
    int new_x = (int)(offset_x * scale_x) + center.x;
    int new_y = (int)(offset_y * scale_y) + center.y;

    dest->points[i] = GPoint(new_x, new_y);
  }
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  //Recalculate scale factors (same as init)
  float scale_x = (float)bounds.size.w / 144.0f; //Base widht for Pebble Time
  float scale_y = (float)bounds.size.h / 168.0f; //Base height for Pebble Time

  //setup the weekday label with scalled position
  int scalled_day_x = (int)(BASE_DAY_LABEL_OFFSET_x * scale_x) + center.x;
  int scalled_day_y = (int)(BASE_DAY_LABEL_OFFSET_y * scale_y) + center.y;
  int scalled_day_height = (int)(BASE_LABEL_HEIGHT * scale_y) + center.y;
  s_day_label = text_layer_create(GRect(scalled_day_x, scalled_day_y, BASE_LABEL_WIDTH_DAY, scalled_day_height));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorBlack);
  text_layer_set_text_color(s_day_label, GColorChromeYellow);
  text_layer_set_font(s_day_label, fonts_get_system_font(DAY_LABEL_FONT));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  //setup the day of the month label 
  int scalled_num_x = (int)(BASE_NUM_LABEL_OFFSET_x * scale_x) + center.x;
  int scalled_num_y = (int)(BASE_NUM_LABEL_OFFSET_y * scale_y) + center.y;
  int scalled_num_width = (int)(BASE_LABEL_WIDTH_NUM * scale_x) + center.x;
  int scalled_num_height = (int)(BASE_LABEL_HEIGHT * scale_y) + center.y;
  s_num_label = text_layer_create(GRect(scalled_num_x, scalled_num_y, scalled_num_width, scalled_num_height));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorBlack);
  text_layer_set_font(s_num_label, fonts_get_system_font(NUM_LABEL_FONT));

  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);

  //setup battery layer
  s_battery_layer = layer_create(bounds);
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  //add to window
  layer_add_child(window_layer, s_battery_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  //Calculate scale factors and adjust tick points for current screen size
  float scale_x = (float)bounds.size.w / 144.0f; //Base widht for Pebble Time
  float scale_y = (float)bounds.size.h / 168.0f; //Base height for Pebble Time

  // init hand paths
  scale_gpath_info(&MINUTE_HAND_POINTS, &s_adjusted_minute_hand_points[0], scale_x, scale_y, center);
  s_minute_arrow = gpath_create(&s_adjusted_minute_hand_points[0]);
  scale_gpath_info(&HOUR_HAND_POINTS, &s_adjusted_hour_hand_points[0], scale_x, scale_y, center);
  s_hour_arrow = gpath_create(&s_adjusted_hour_hand_points[0]);
  
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    scale_gpath_info(&ANALOG_BG_POINTS[i], &s_adjusted_bg_points[i], scale_x, scale_y, center);
    s_tick_paths[i] = gpath_create(&s_adjusted_bg_points[i]);
  } 

  //init date box path
  scale_gpath_info(&DATE_BOX_POINTS, &s_adjusted_date_box_points[0], scale_x, scale_y, center);
  s_date_box = gpath_create(&s_adjusted_date_box_points[0]);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  //register for battery level updates
  battery_state_service_subscribe(battery_callback);
  //ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);
  gpath_destroy(s_date_box);

  free(s_adjusted_date_box_points[0].points); // Free the allocated points array for date box
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    free(s_adjusted_bg_points[i].points); // Free the allocated points array
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}