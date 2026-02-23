#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;

// Day abbreviations matching the real F-91W
static const char *DAYS[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};

// ---- Color definitions ----
#ifdef PBL_COLOR
  #define COLOR_LCD_BG     GColorFromHEX(0xB6C08C)
  #define COLOR_LCD_FG     GColorFromHEX(0x181C12)
  #define COLOR_LCD_GHOST  GColorFromHEX(0xAAB682)
  #define COLOR_BEZEL      GColorFromHEX(0x303236)
  #define COLOR_BEZEL_HI   GColorFromHEX(0x484C52)
  #define COLOR_BLUE       GColorFromHEX(0x14328C)
  #define COLOR_DIM        GColorFromHEX(0x5A5C60)
  #define COLOR_RED        GColorFromHEX(0xAA231E)
  #define COLOR_CASIO_BLUE GColorFromHEX(0x193787)
  #define COLOR_WHITE      GColorWhite
  #define COLOR_BLACK       GColorBlack
#else
  #define COLOR_LCD_BG     GColorWhite
  #define COLOR_LCD_FG     GColorBlack
  #define COLOR_LCD_GHOST  GColorLightGray
  #define COLOR_BEZEL      GColorBlack
  #define COLOR_BEZEL_HI   GColorDarkGray
  #define COLOR_BLUE       GColorBlack
  #define COLOR_DIM        GColorDarkGray
  #define COLOR_RED         GColorBlack
  #define COLOR_CASIO_BLUE GColorBlack
  #define COLOR_WHITE      GColorWhite
  #define COLOR_BLACK       GColorBlack
#endif

// Layout constants (calculated relative to screen)
static int16_t BEZEL_PAD;
static int16_t LCD_X, LCD_Y, LCD_W, LCD_H;
static int16_t BLUE_PAD;

static void calculate_layout(GRect bounds) {
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  BEZEL_PAD = 4;
  BLUE_PAD = 2;

  LCD_X = 18;
  LCD_Y = 56;
  LCD_W = w - 36;
  LCD_H = 120;

  // Adjust for round screens
  #ifdef PBL_ROUND
    LCD_X = 30;
    LCD_Y = 56;
    LCD_W = w - 60;
    LCD_H = 110;
  #endif

  // Adjust for Emery (228x228)
  #ifdef PBL_DISPLAY_LARGE
    LCD_X = 22;
    LCD_Y = 64;
    LCD_W = w - 44;
    LCD_H = 130;
  #endif
}

static void draw_bezel(GContext *ctx, GRect bounds) {
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  // Black outer
  graphics_context_set_fill_color(ctx, COLOR_BLACK);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Dark bezel body
  graphics_context_set_fill_color(ctx, COLOR_BEZEL);
  graphics_fill_rect(ctx, GRect(BEZEL_PAD, BEZEL_PAD,
    w - BEZEL_PAD * 2, h - BEZEL_PAD * 2), 0, GCornerNone);

  // Highlight edges
  graphics_context_set_fill_color(ctx, COLOR_BEZEL_HI);
  graphics_fill_rect(ctx, GRect(6, 6, w - 12, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(6, 6, 1, h - 12), 0, GCornerNone);
}

static void draw_bezel_text(GContext *ctx, GRect bounds) {
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GFont font_tiny = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  // "CASIO" top center
  GSize casio_size = graphics_text_layout_get_content_size(
    "CASIO", font_small, GRect(0, 0, w, 30),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "CASIO", font_small,
    GRect(0, 6, w, 24),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // "LIGHT" top left
  graphics_context_set_text_color(ctx, COLOR_DIM);
  graphics_draw_text(ctx, "LIGHT", font_tiny,
    GRect(10, 8, 60, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // "F-91W" top right
  graphics_draw_text(ctx, "F-91W", font_tiny,
    GRect(w - 70, 8, 60, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // "ALARM CHRONOGRAPH" below top row
  graphics_draw_text(ctx, "ALARM CHRONOGRAPH", font_tiny,
    GRect(0, 32, w, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Bottom bezel: WATER  WR  RESIST
  int16_t bot_y = LCD_Y + LCD_H + BLUE_PAD + 8;

  graphics_draw_text(ctx, "WATER", font_tiny,
    GRect(14, bot_y, 60, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Red "W"
  graphics_context_set_text_color(ctx, COLOR_RED);
  graphics_draw_text(ctx, "W", font_small,
    GRect(w / 2 - 18, bot_y - 2, 20, 22),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Blue "R"
  graphics_context_set_text_color(ctx, COLOR_CASIO_BLUE);
  graphics_draw_text(ctx, "R", font_small,
    GRect(w / 2 + 2, bot_y - 2, 20, 22),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // "RESIST"
  graphics_context_set_text_color(ctx, COLOR_DIM);
  graphics_draw_text(ctx, "RESIST", font_tiny,
    GRect(w - 74, bot_y, 60, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

static void draw_lcd_panel(GContext *ctx) {
  // Blue border
  graphics_context_set_fill_color(ctx, COLOR_BLUE);
  graphics_fill_rect(ctx,
    GRect(LCD_X - BLUE_PAD, LCD_Y - BLUE_PAD,
          LCD_W + BLUE_PAD * 2, LCD_H + BLUE_PAD * 2),
    0, GCornerNone);

  // LCD background
  graphics_context_set_fill_color(ctx, COLOR_LCD_BG);
  graphics_fill_rect(ctx, GRect(LCD_X, LCD_Y, LCD_W, LCD_H), 0, GCornerNone);
}

static void draw_lcd_content(GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  GFont font_big = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  GFont font_med = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  int16_t cx = LCD_X + 8;
  int16_t cw = LCD_W - 16;

  graphics_context_set_text_color(ctx, COLOR_LCD_FG);

  // === TOP ROW: Day + Date ===
  int16_t top_y = LCD_Y + 4;

  // Day of week
  graphics_draw_text(ctx, DAYS[t->tm_wday], font_small,
    GRect(cx, top_y, 40, 22),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Date number
  static char date_buf[4];
  snprintf(date_buf, sizeof(date_buf), "%02d", t->tm_mday);
  graphics_draw_text(ctx, date_buf, font_small,
    GRect(LCD_X + LCD_W - 40, top_y, 32, 22),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // Divider line
  int16_t div1_y = top_y + 22;
  graphics_context_set_stroke_color(ctx, COLOR_LCD_FG);
  graphics_draw_line(ctx,
    GPoint(LCD_X + 3, div1_y),
    GPoint(LCD_X + LCD_W - 3, div1_y));

  // === MAIN TIME ===
  int16_t time_y = div1_y + 4;

  static char time_buf[6];
  strftime(time_buf, sizeof(time_buf),
    clock_is_24h_style() ? "%H:%M" : "%I:%M", t);

  // Draw time left-aligned with room for seconds
  graphics_draw_text(ctx, time_buf, font_big,
    GRect(cx - 4, time_y - 6, cw - 30, 50),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Seconds to the right, aligned to bottom of time
  static char sec_buf[4];
  snprintf(sec_buf, sizeof(sec_buf), "%02d", t->tm_sec);

  int16_t sec_y = time_y + 22;
  graphics_draw_text(ctx, sec_buf, font_small,
    GRect(LCD_X + LCD_W - 36, sec_y, 28, 22),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  calculate_layout(bounds);

  draw_bezel(ctx, bounds);
  draw_bezel_text(ctx, bounds);
  draw_lcd_panel(ctx);
  draw_lcd_content(ctx);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
