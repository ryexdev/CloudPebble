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
  #define COLOR_GOLD       GColorFromHEX(0xC8A850)
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
  #define COLOR_GOLD       GColorLightGray
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

  // Blue outer border
  graphics_context_set_fill_color(ctx, COLOR_BLUE);
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

  GFont font_casio = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  GFont font_tiny = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  // Row 1: "CASIO" upper-left, "F-91W" upper-right
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "CASIO", font_casio,
    GRect(LCD_X, 6, LCD_W / 2, 24),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, COLOR_GOLD);
  graphics_draw_text(ctx, "F-91W", font_small,
    GRect(LCD_X + LCD_W / 2, 10, LCD_W / 2, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // Row 2: Blue stripe is the blue border drawn in draw_lcd_panel

  // Row 3: Red arrow + "LIGHT" on left, "ALARM CHRONOGRAPH" on right
  int16_t row3_y = 37;

  // Small red left-pointing arrow (triangle approximation using rects)
  graphics_context_set_fill_color(ctx, COLOR_RED);
  graphics_fill_rect(ctx, GRect(LCD_X, row3_y + 6, 5, 3), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + 1, row3_y + 5, 3, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + 2, row3_y + 4, 1, 7), 0, GCornerNone);

  // "LIGHT" to the right of the arrow
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "LIGHT", font_tiny,
    GRect(LCD_X + 8, row3_y, 50, 16),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // "ALARM CHRONOGRAPH" right side
  graphics_context_set_text_color(ctx, COLOR_GOLD);
  graphics_draw_text(ctx, "ALARM CHRONOGRAPH", font_tiny,
    GRect(LCD_X, row3_y, LCD_W, 16),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // === BELOW LCD ===
  int16_t below_base = LCD_Y + LCD_H + BLUE_PAD;

  // Row 1: Red indicator + "MODE" left, "ALARM ON-OFF/24HR" + red indicator right
  int16_t brow1_y = below_base + 3;

  // Red indicator (left, small triangle pointing down)
  graphics_context_set_fill_color(ctx, COLOR_RED);
  graphics_fill_rect(ctx, GRect(LCD_X, brow1_y + 6, 5, 3), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + 1, brow1_y + 5, 3, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + 2, brow1_y + 4, 1, 7), 0, GCornerNone);

  // "MODE"
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "MODE", font_tiny,
    GRect(LCD_X + 8, brow1_y, 50, 16),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // "ALARM ON-OFF/24HR" right-aligned
  graphics_draw_text(ctx, "ALARM ON\xc2\xb7OFF/24HR", font_tiny,
    GRect(LCD_X, brow1_y, LCD_W - 8, 16),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // Red indicator (right side, small triangle pointing down)
  graphics_context_set_fill_color(ctx, COLOR_RED);
  graphics_fill_rect(ctx, GRect(LCD_X + LCD_W - 5, brow1_y + 6, 5, 3), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + LCD_W - 4, brow1_y + 5, 3, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(LCD_X + LCD_W - 3, brow1_y + 4, 1, 7), 0, GCornerNone);

  // Row 2: Blue line (same spacing as top)
  int16_t brow2_y = brow1_y + 17;
  graphics_context_set_fill_color(ctx, COLOR_BLUE);
  graphics_fill_rect(ctx,
    GRect(LCD_X - BLUE_PAD, brow2_y,
          LCD_W + BLUE_PAD * 2, 4),
    0, GCornerNone);

  // Row 3: "WATER" left, blue rect with "WR", "RESIST" right
  int16_t brow3_y = brow2_y + 7;

  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "WATER", font_tiny,
    GRect(LCD_X, brow3_y, 60, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Blue rectangle with "WR" centered
  int16_t wr_w = 28;
  int16_t wr_h = 16;
  int16_t wr_x = (w - wr_w) / 2;
  int16_t wr_y = brow3_y + 1;
  graphics_context_set_fill_color(ctx, COLOR_CASIO_BLUE);
  graphics_fill_rect(ctx, GRect(wr_x, wr_y, wr_w, wr_h), 0, GCornerNone);
  graphics_context_set_text_color(ctx, COLOR_RED);
  graphics_draw_text(ctx, "WR", font_small,
    GRect(wr_x, wr_y - 2, wr_w, wr_h),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // "RESIST"
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "RESIST", font_tiny,
    GRect(LCD_X, brow3_y, LCD_W, 20),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

static void draw_lcd_panel(GContext *ctx) {
  // Blue stripe between CASIO row and LIGHT row
  graphics_context_set_fill_color(ctx, COLOR_BLUE);
  graphics_fill_rect(ctx,
    GRect(LCD_X - BLUE_PAD, 30,
          LCD_W + BLUE_PAD * 2, 4),
    0, GCornerNone);

  // Blue border around LCD
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

// Core: draw arbitrary segment pattern
static void draw_seg_pattern(GContext *ctx, uint8_t segs, int16_t x, int16_t y,
                             int16_t sw, int16_t sh, int16_t st) {
  int16_t g = 1;
  int16_t vl = (sh - 3 * st) / 2;
  int16_t my = y + st + vl;
  int16_t by = y + sh - st;

  if (segs & 0x01) graphics_fill_rect(ctx, GRect(x+g, y, sw-2*g, st), 0, GCornerNone);
  if (segs & 0x40) graphics_fill_rect(ctx, GRect(x+g, my, sw-2*g, st), 0, GCornerNone);
  if (segs & 0x08) graphics_fill_rect(ctx, GRect(x+g, by, sw-2*g, st), 0, GCornerNone);
  if (segs & 0x20) graphics_fill_rect(ctx, GRect(x, y+st+g, st, vl-2*g), 0, GCornerNone);
  if (segs & 0x02) graphics_fill_rect(ctx, GRect(x+sw-st, y+st+g, st, vl-2*g), 0, GCornerNone);
  if (segs & 0x10) graphics_fill_rect(ctx, GRect(x, my+st+g, st, vl-2*g), 0, GCornerNone);
  if (segs & 0x04) graphics_fill_rect(ctx, GRect(x+sw-st, my+st+g, st, vl-2*g), 0, GCornerNone);
}

static void draw_seg_digit(GContext *ctx, int d, int16_t x, int16_t y,
                           int16_t sw, int16_t sh, int16_t st) {
  if (d < 0 || d > 9) return;
  draw_seg_pattern(ctx, SEG_MAP[d], x, y, sw, sh, st);
}

// 7-segment letter approximations for day/AM/PM
static uint8_t letter_segs(char c) {
  switch(c) {
    case 'A': return 0x77; // a,b,c,e,f,g
    case 'E': return 0x79; // a,d,e,f,g
    case 'F': return 0x71; // a,e,f,g
    case 'H': return 0x76; // b,c,e,f,g
    case 'M': return 0x37; // a,b,c,e,f
    case 'O': return 0x3F; // a,b,c,d,e,f
    case 'P': return 0x73; // a,b,e,f,g
    case 'R': return 0x50; // e,g
    case 'S': return 0x6D; // a,c,d,f,g
    case 'T': return 0x78; // d,e,f,g
    case 'U': return 0x3E; // b,c,d,e,f
    case 'W': return 0x1C; // c,d,e
    default:  return 0x00;
  }
}

static void draw_seg_colon(GContext *ctx, int16_t x, int16_t y, int16_t sh, int16_t st) {
  int16_t q = sh / 4;
  graphics_fill_rect(ctx, GRect(x, y + q, st, st), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x, y + 3 * q - st, st, st), 0, GCornerNone);
}

static void draw_seg_dash(GContext *ctx, int16_t x, int16_t y,
                          int16_t sw, int16_t sh, int16_t st) {
  int16_t vl = (sh - 3 * st) / 2;
  int16_t my = y + st + vl;
  graphics_fill_rect(ctx, GRect(x, my, sw, st), 0, GCornerNone);
}

static void draw_lcd_content(GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  graphics_context_set_fill_color(ctx, COLOR_LCD_FG);

  // Main time digit sizes (bigger, bolder)
  int16_t dw = 24, dh = 42, dt = 5;
  // Seconds digit sizes
  int16_t secw = 14, sech = 24, sect = 3;
  // Top row character sizes (day, date)
  int16_t tw = 10, th = 17, tt = 2;
  // Spacing
  int16_t dgap = 2, cgap = 2, colw = 5;
  int16_t secgap = 1, secpad = 4;
  int16_t tgap = 1, dashw = 5;

  // Total width: HH:MM + padding + SS
  int16_t time_w = 4 * dw + 2 * dgap + 2 * cgap + colw;
  int16_t secs_w = 2 * secw + secgap;
  int16_t total_w = time_w + secpad + secs_w;

  // Vertical centering
  int16_t top_row_h = 22;
  int16_t div_gap = 4;
  int16_t content_h = top_row_h + div_gap + dh;
  int16_t top_y = LCD_Y + (LCD_H - content_h) / 2;
  int16_t char_y = top_y + (top_row_h - th) / 2;

  // === TOP ROW ===
  // Day of week - centered, 7-segment
  const char *day = DAYS[t->tm_wday];
  int16_t day_w = 2 * tw + tgap;
  int16_t day_x = LCD_X + (LCD_W - day_w) / 2;
  draw_seg_pattern(ctx, letter_segs(day[0]), day_x, char_y, tw, th, tt);
  day_x += tw + tgap;
  draw_seg_pattern(ctx, letter_segs(day[1]), day_x, char_y, tw, th, tt);

  // Date - right side, 7-segment digits
  int16_t month = t->tm_mon + 1;
  int16_t mday = t->tm_mday;
  int16_t m_digits = month >= 10 ? 2 : 1;
  int16_t date_total_w = (m_digits + 2) * tw + dashw + (m_digits + 2) * tgap;
  int16_t dx = LCD_X + LCD_W - 4 - date_total_w;

  if (month >= 10) {
    draw_seg_digit(ctx, month / 10, dx, char_y, tw, th, tt);
    dx += tw + tgap;
  }
  draw_seg_digit(ctx, month % 10, dx, char_y, tw, th, tt);
  dx += tw + tgap;
  draw_seg_dash(ctx, dx, char_y, dashw, th, tt);
  dx += dashw + tgap;
  draw_seg_digit(ctx, mday / 10, dx, char_y, tw, th, tt);
  dx += tw + tgap;
  draw_seg_digit(ctx, mday % 10, dx, char_y, tw, th, tt);

  // === MAIN TIME (7-segment, centered) ===
  int16_t time_y = top_y + top_row_h + div_gap;

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
  cx += colw + cgap;

  // M1
  draw_seg_digit(ctx, m1, cx, time_y, dw, dh, dt);
  cx += dw + dgap;

  // M2
  draw_seg_digit(ctx, m2, cx, time_y, dw, dh, dt);
  cx += dw + secpad;

  // Seconds - bottom-aligned with main time, 7-segment
  int16_t sec_y = time_y + dh - sech;
  draw_seg_digit(ctx, s1, cx, sec_y, secw, sech, sect);
  cx += secw + secgap;
  draw_seg_digit(ctx, s2, cx, sec_y, secw, sech, sect);

  // AM/PM - text font, left side of main time area
  GFont font_label = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  graphics_context_set_text_color(ctx, COLOR_LCD_FG);
  graphics_draw_text(ctx, t->tm_hour < 12 ? "AM" : "PM", font_label,
    GRect(LCD_X + 4, time_y, 30, 16),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
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
