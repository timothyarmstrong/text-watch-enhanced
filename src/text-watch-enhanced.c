#include <pebble.h>

enum {
  KEY_FETCH = 0x1,
  KEY_TEMPERATURE = 0x2,
  KEY_CITY = 0x3
};

static Window* window;
static TextLayer* time_hour_layer;
static TextLayer* time_minute_line_one_layer;
static TextLayer* time_minute_line_two_layer;
static TextLayer* date_layer;
static TextLayer* temperature_layer;
// Somewhere to hold the string version of the date. Looks like:
// Thurs. July 31
static char date_buffer[32];
static char temperature_buffer[16];

typedef struct {
  char* line_one;
  char* line_two;
} MinuteText;

static char* const ONES[] = {
  "",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine"
};

static MinuteText const TEENS[] = {
  { "ten", "" },
  { "eleven", "" },
  { "twelve", "" },
  { "thirteen", "" },
  { "four", "teen" },
  { "fifteen", "" },
  { "sixteen", "" },
  { "seven", "teen" },
  { "eight", "teen" },
  { "nine", "teen" },
};

static char* const TENS[] = {
  "oh",
  "",
  "twenty",
  "thirty",
  "forty",
  "fifty",
};

static char* const OCLOCK = "o'clock";

static char* const DAYS[] = {
  "Sun.",
  "Mon.",
  "Tues.",
  "Wed.",
  "Thurs.",
  "Fri.",
  "Sat."
};

static char* const MONTHS[] = {
  "Jan.",
  "Feb.",
  "Mar.",
  "Apr.",
  "May",
  "June",
  "July",
  "Aug.",
  "Sep.",
  "Oct.",
  "Nov.",
  "Dec."
};

static char* const EMPTY = "";

static void get_minute_text(int minute, MinuteText* text) {
  // For times on the hour.
  if (minute == 0) {
    text->line_one = OCLOCK;
    text->line_two = EMPTY;
    return;
  }

  // For times in the teens.
  if (minute >= 10 && minute <= 19) {
    text->line_one = TEENS[minute - 10].line_one;
    text->line_two = TEENS[minute - 10].line_two;
    return;
  }

  // For all other times.
  text->line_one = TENS[minute / 10];
  text->line_two = ONES[minute % 10];
}

static char* get_hour_text(int hour) {
  if (hour == 0) {
    // "twelve"
    return TEENS[2].line_one;
  }
  if (hour <= 9) {
    return ONES[hour];
  }
  return TEENS[hour - 10].line_one;
}

static void update_time(struct tm* time) {
  int hour = time->tm_hour;
  // Enforce 12-hour time.
  if (hour >= 13) {
    hour -= 12;
  }
  int minute = time->tm_min;

  int day = time->tm_wday;
  int date = time->tm_mday;
  int month = time->tm_mon;

  char* hour_text = get_hour_text(hour);

  MinuteText minute_text;
  get_minute_text(minute, &minute_text);

  char* day_text = DAYS[day];
  char* month_text = MONTHS[month];

  snprintf(date_buffer, 32, "%s %s %d", day_text, month_text, date);
  //snprintf(date_buffer, 32, "Thurs. Mar. 28");

  text_layer_set_text(time_hour_layer, hour_text);
  text_layer_set_text(time_minute_line_one_layer, minute_text.line_one);
  text_layer_set_text(time_minute_line_two_layer, minute_text.line_two);
  text_layer_set_text(date_layer, date_buffer);

  // Update the weather every 30 minutes.
  if (minute % 30 == 0) {
    fetch_weather();
  }
}

static void update_weather(int temperature) {
  snprintf(temperature_buffer, 16, "%d°C", temperature);
  text_layer_set_text(temperature_layer, temperature_buffer);
}

static void handle_message_received(DictionaryIterator* iter, void* context) {
  Tuple* temperature_tuple = dict_find(iter, KEY_TEMPERATURE);
  Tuple* city_tuple = dict_find(iter, KEY_CITY);

  int temperature;
  char* city;
  if (temperature_tuple) {
    temperature = temperature_tuple->value->int32;
    update_weather(temperature);
  }
}

// Called when a message in the inbox is dropped.
static void handle_message_dropped(AppMessageResult reason, void* context) {
  // TODO: Log this, and maybe show to user.
}


static void handle_message_failed(DictionaryIterator* iter,
                                  AppMessageResult reason, void* context) {
  // TODO: Log this, and maybe show to user.
}

static void fetch_weather() {
  Tuplet fetch_tuplet = TupletInteger(KEY_FETCH, 1);

  DictionaryIterator* iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    // TODO: Logging.
    return;
  }

  dict_write_tuplet(iter, &fetch_tuplet);
  dict_write_end(iter);

  app_message_outbox_send();

  // TODO: Show loading?
}

static void app_message_init() {
  app_message_register_inbox_received(handle_message_received);
  app_message_register_inbox_dropped(handle_message_dropped);
  app_message_register_outbox_failed(handle_message_failed);

  app_message_open(256, 256);
}

static void handle_minute_tick(struct tm* time, TimeUnits units_changed) {
  update_time(time);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {

}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {

}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {

}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  time_hour_layer = text_layer_create((GRect) {
    .origin = {0, 0},
    .size = {bounds.size.w, 50}
  });
  text_layer_set_background_color(time_hour_layer, GColorClear);
  text_layer_set_text_color(time_hour_layer, GColorWhite);
  text_layer_set_font(time_hour_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(time_hour_layer));

  time_minute_line_one_layer = text_layer_create((GRect) {
    .origin = {0, 35},
    .size = {bounds.size.w, 50}
  });
  text_layer_set_background_color(time_minute_line_one_layer, GColorClear);
  text_layer_set_text_color(time_minute_line_one_layer, GColorWhite);
  text_layer_set_font(time_minute_line_one_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(time_minute_line_one_layer));

  time_minute_line_two_layer = text_layer_create((GRect) {
    .origin = {0, 70},
    .size = {bounds.size.w, 50}
  });
  text_layer_set_background_color(time_minute_line_two_layer, GColorClear);
  text_layer_set_text_color(time_minute_line_two_layer, GColorWhite);
  text_layer_set_font(time_minute_line_two_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(time_minute_line_two_layer));

  date_layer = text_layer_create((GRect) {
    .origin = {5, 145},
    .size = {bounds.size.w - 40, 25}
  });
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  temperature_layer = text_layer_create((GRect) {
    .origin = {bounds.size.w - 40, 145},
    .size = {40, 25}
  });
  text_layer_set_background_color(temperature_layer, GColorClear);
  text_layer_set_text_color(temperature_layer, GColorWhite);
  text_layer_set_font(temperature_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(temperature_layer));

  // Set the initial time.
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);

  fetch_weather();
}

static void window_unload(Window *window) {
  text_layer_destroy(time_hour_layer);
  text_layer_destroy(time_minute_line_one_layer);
  text_layer_destroy(time_minute_line_two_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(temperature_layer);
}

static void init(void) {
  app_message_init();

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  // Hide the status bar.
  window_set_fullscreen(window, true);

  window_set_background_color(window, GColorBlack);

  // The display only needs to be updated once per minute.
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
