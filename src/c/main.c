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

// === 7-segment display rendering ===

// Segment bitmask: a=top, b=upper-right, c=lower-right, d=bottom, e=lower-left, f=upper-left, g=middle
static const uint8_t SEG_MAP[] = {
  0x3F, // 0: a,b,c,d,e,f
  0x06, // 1: b,c
  0x5B, // 2: a,b,d,e,g
  0x4F, // 3: a,b,c,d,g
  0x66, // 4: b,c,f,g
  0x6D, // 5: a,c,d,f,g
  0x7D, // 6: a,c,d,e,f,g
  0x07, // 7: a,b,c
  0x7F, // 8: all
  0x6F, // 9: a,b,c,d,f,g
};

static void draw_seg_digit(GContext *ctx, int d, int16_t x, int16_t y,
                           int16_t w, int16_t h, int16_t t) {
  if (d < 0 || d > 9) return;
  uint8_t s = SEG_MAP[d];
  int16_t g = 1;
  int16_t vl = (h - 3 * t) / 2;
  int16_t my = y + t + vl;
  int16_t by = y + h - t;

  if (s & 0x01) graphics_fill_rect(ctx, GRect(x+g, y, w-2*g, t), 0, GCornerNone);
  if (s & 0x40) graphics_fill_rect(ctx, GRect(x+g, my, w-2*g, t), 0, GCornerNone);
  if (s & 0x08) graphics_fill_rect(ctx, GRect(x+g, by, w-2*g, t), 0, GCornerNone);
  if (s & 0x20) graphics_fill_rect(ctx, GRect(x, y+t+g, t, vl-2*g), 0, GCornerNone);
  if (s & 0x02) graphics_fill_rect(ctx, GRect(x+w-t, y+t+g, t, vl-2*g), 0, GCornerNone);
  if (s & 0x10) graphics_fill_rect(ctx, GRect(x, my+t+g, t, vl-2*g), 0, GCornerNone);
  if (s & 0x04) graphics_fill_rect(ctx, GRect(x+w-t, my+t+g, t, vl-2*g), 0, GCornerNone);
}

static void draw_seg_colon(GContext *ctx, int16_t x, int16_t y, int16_t h, int16_t t) {
  int16_t q = h / 4;
  graphics_fill_rect(ctx, GRect(x, y + q, t, t), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x, y + 3 * q - t, t, t), 0, GCornerNone);
}

static void draw_seg_dash(GContext *ctx, int16_t x, int16_t y,
                          int16_t w, int16_t h, int16_t t) {
  int16_t vl = (h - 3 * t) / 2;
  int16_t my = y + t + vl;
  graphics_fill_rect(ctx, GRect(x, my, w, t), 0, GCornerNone);
}

static void draw_lcd_content(GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  GFont font_label = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  graphics_context_set_text_color(ctx, COLOR_LCD_FG);
  graphics_context_set_fill_color(ctx, COLOR_LCD_FG);

  // Main time digit sizes (bigger, bolder)
  int16_t dw = 24, dh = 42, dt = 5;
  // Seconds digit sizes
  int16_t sw = 14, sh = 24, st = 3;
  // Date digit sizes
  int16_t ddw = 9, ddh = 16, ddt = 2;
  // Spacing
  int16_t dgap = 2, cgap = 2, cw = 5;
  int16_t sgap = 1, spad = 4;
  int16_t ddgap = 1, dash_w = 6;

  // Total width: HH:MM + padding + SS
  int16_t time_w = 4 * dw + 2 * dgap + 2 * cgap + cw;
  int16_t secs_w = 2 * sw + sgap;
  int16_t total_w = time_w + spad + secs_w;

  // Vertical centering
  int16_t top_row_h = 20;
  int16_t div_gap = 4;
  int16_t content_h = top_row_h + div_gap + dh;
  int16_t top_y = LCD_Y + (LCD_H - content_h) / 2;

  // === TOP ROW ===
  // Day of week - centered (text font since these are letters)
  graphics_draw_text(ctx, DAYS[t->tm_wday], font_label,
    GRect(LCD_X, top_y, LCD_W, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // AM/PM - left side
  graphics_draw_text(ctx, t->tm_hour < 12 ? "AM" : "PM", font_label,
    GRect(LCD_X + 4, top_y, 30, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Date - right side, 7-segment digits
  int16_t month = t->tm_mon + 1;
  int16_t mday = t->tm_mday;
  int16_t m_digits = month >= 10 ? 2 : 1;
  int16_t date_total_w = (m_digits + 2) * ddw + dash_w + (m_digits + 2) * ddgap;
  int16_t date_y = top_y + (top_row_h - ddh) / 2;
  int16_t dx = LCD_X + LCD_W - 6 - date_total_w;

  if (month >= 10) {
    draw_seg_digit(ctx, month / 10, dx, date_y, ddw, ddh, ddt);
    dx += ddw + ddgap;
  }
  draw_seg_digit(ctx, month % 10, dx, date_y, ddw, ddh, ddt);
  dx += ddw + ddgap;
  draw_seg_dash(ctx, dx, date_y, dash_w, ddh, ddt);
  dx += dash_w + ddgap;
  draw_seg_digit(ctx, mday / 10, dx, date_y, ddw, ddh, ddt);
  dx += ddw + ddgap;
  draw_seg_digit(ctx, mday % 10, dx, date_y, ddw, ddh, ddt);

  // Divider line
  int16_t div_y = top_y + top_row_h;
  graphics_context_set_stroke_color(ctx, COLOR_LCD_FG);
  graphics_draw_line(ctx,
    GPoint(LCD_X + 3, div_y),
    GPoint(LCD_X + LCD_W - 3, div_y));

  // === MAIN TIME (7-segment, centered) ===
  int16_t time_y = div_y + div_gap;

  // 12-hour format always
  int16_t hour = t->tm_hour % 12;
  if (hour == 0) hour = 12;
  int16_t h1 = hour / 10;
  int16_t h2 = hour % 10;
  int16_t m1 = t->tm_min / 10;
  int16_t m2 = t->tm_min % 10;
  int16_t s1 = t->tm_sec / 10;
  int16_t s2 = t->tm_sec % 10;

  // Center the whole time+seconds group horizontally
  int16_t cx = LCD_X + (LCD_W - total_w) / 2;

  // H1 (blank for hours 1-9, like real F-91W)
  if (h1 > 0) draw_seg_digit(ctx, h1, cx, time_y, dw, dh, dt);
  cx += dw + dgap;

  // H2
  draw_seg_digit(ctx, h2, cx, time_y, dw, dh, dt);
  cx += dw + cgap;

  // Colon
  draw_seg_colon(ctx, cx, time_y, dh, dt);
  cx += cw + cgap;

  // M1
  draw_seg_digit(ctx, m1, cx, time_y, dw, dh, dt);
  cx += dw + dgap;

  // M2
  draw_seg_digit(ctx, m2, cx, time_y, dw, dh, dt);
  cx += dw + spad;

  // Seconds - bottom-aligned with main time, 7-segment
  int16_t sec_y = time_y + dh - sh;
  draw_seg_digit(ctx, s1, cx, sec_y, sw, sh, st);
  cx += sw + sgap;
  draw_seg_digit(ctx, s2, cx, sec_y, sw, sh, st);
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
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
