#include <pebble.h>

/* ── Geometry ─────────────────────────────────────────────────────────────── */
#define CELL        14    /* digit brick pixel size                           */
#define BG_CELL     10    /* background brick pixel size                      */
#define DIG_STUD     3    /* digit brick stud radius                          */
#define BG_STUD      2    /* background brick stud radius                     */
#define ANIM_MS     90    /* milliseconds between animation row reveals       */

/*
 * Horizontal layout (200 px wide, emery):
 *   3 + D(42) + 3 + D(42) + 3 + C(14) + 3 + D(42) + 3 + D(42) + 3 = 200
 */
#define DIG0_X    3
#define DIG1_X   48   /* 3 + 42 + 3                  */
#define COLON_X  93   /* 48 + 42 + 3, width = CELL   */
#define DIG2_X  110   /* 93 + 14 + 3                 */
#define DIG3_X  155   /* 110 + 42 + 3                */

/* Vertically centre the 70-px tall (5 × CELL) digit block: (228 - 70) / 2 */
#define TIME_Y   79

/* ── 3×5 digit bitmaps (bit 2 = left col, bit 0 = right col) ─────────────── */
static const uint8_t DIGIT_MAP[10][5] = {
    { 0b111, 0b101, 0b101, 0b101, 0b111 }, /* 0 */
    { 0b010, 0b110, 0b010, 0b010, 0b111 }, /* 1 */
    { 0b111, 0b001, 0b111, 0b100, 0b111 }, /* 2 */
    { 0b111, 0b001, 0b111, 0b001, 0b111 }, /* 3 */
    { 0b101, 0b101, 0b111, 0b001, 0b001 }, /* 4 */
    { 0b111, 0b100, 0b111, 0b001, 0b111 }, /* 5 */
    { 0b111, 0b100, 0b111, 0b101, 0b111 }, /* 6 */
    { 0b111, 0b001, 0b001, 0b001, 0b001 }, /* 7 */
    { 0b111, 0b101, 0b111, 0b101, 0b111 }, /* 8 */
    { 0b111, 0b101, 0b111, 0b001, 0b111 }, /* 9 */
};

/* ── Globals ──────────────────────────────────────────────────────────────── */
static Window   *s_window;
static Layer    *s_canvas;
static AppTimer *s_anim_timer;
static int       s_hours;
static int       s_minutes;
/* Per-digit: rows 0..s_anim_rows[d]-1 are visible; 5 = fully shown (static) */
static int       s_anim_rows[4];

/* ── Colour helpers ───────────────────────────────────────────────────────── */

/* Reduce each 2-bit channel by 1 (clamp at 0) for a darker shade. */
static GColor darken(GColor c) {
    GColor out;
    out.a = 3;
    out.r = c.r > 0 ? (uint8_t)(c.r - 1) : 0;
    out.g = c.g > 0 ? (uint8_t)(c.g - 1) : 0;
    out.b = c.b > 0 ? (uint8_t)(c.b - 1) : 0;
    return out;
}

/*
 * Eight classic brick-toys colours — no whites/greys so white digit bricks
 * stand out clearly against the background.
 */
static GColor bg_brick_color(int col, int row) {
    static const uint8_t R[8] = { 255,   0, 255,   0, 255, 170,   0, 255 };
    static const uint8_t G[8] = {   0,  85, 255, 170,  85,   0, 170,   0 };
    static const uint8_t B[8] = {   0, 255,   0,   0,   0, 255, 170, 170 };
    int i = ((col * 5) + (row * 7)) % 8;
    return GColorFromRGB(R[i], G[i], B[i]);
}

/* ── Drawing helpers ──────────────────────────────────────────────────────── */

static void draw_bg_brick(GContext *ctx, int x, int y) {
    GColor color = bg_brick_color(x / BG_CELL, y / BG_CELL);
    /* Body — 1 px gap at right/bottom creates the mortar-line grid effect */
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(x, y, BG_CELL - 1, BG_CELL - 1), 0, GCornerNone);
    /* Stud */
    GPoint c = GPoint(x + BG_CELL / 2 - 1, y + BG_CELL / 2 - 1);
    graphics_context_set_fill_color(ctx, darken(color));
    graphics_fill_circle(ctx, c, BG_STUD);
}

static void draw_digit_brick(GContext *ctx, int x, int y, GColor color) {
    /* Body with slight rounded corners for the brick plate look */
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(x, y, CELL - 1, CELL - 1), 2, GCornersAll);
    /* Stud */
    GPoint c = GPoint(x + CELL / 2 - 1, y + CELL / 2 - 1);
    graphics_context_set_fill_color(ctx, darken(color));
    graphics_fill_circle(ctx, c, DIG_STUD);
}

/* ── Canvas update ────────────────────────────────────────────────────────── */

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    /* 1 ── Colourful brick background */
    for (int r = 0; r * BG_CELL < bounds.size.h; r++) {
        for (int c = 0; c * BG_CELL < bounds.size.w; c++) {
            draw_bg_brick(ctx, c * BG_CELL, r * BG_CELL);
        }
    }

    /* 2 ── Time digits (white = ON, dark-grey = OFF) */
    const int DIG_X[4] = { DIG0_X, DIG1_X, DIG2_X, DIG3_X };
    const int DIG_VAL[4] = {
        s_hours   / 10,
        s_hours   % 10,
        s_minutes / 10,
        s_minutes % 10,
    };

    for (int d = 0; d < 4; d++) {
        for (int row = 0; row < 5; row++) {
            if (row >= s_anim_rows[d]) break;
            for (int col = 0; col < 3; col++) {
                bool on = (DIGIT_MAP[DIG_VAL[d]][row] >> (2 - col)) & 1;
                GColor color = on ? GColorWhite : GColorDarkGray;
                draw_digit_brick(ctx,
                    DIG_X[d] + col * CELL,
                    TIME_Y   + row * CELL,
                    color);
            }
        }
    }

    /* 3 ── Colon: always visible (never changes) */
    draw_digit_brick(ctx, COLON_X, TIME_Y + 1 * CELL, GColorWhite);
    draw_digit_brick(ctx, COLON_X, TIME_Y + 3 * CELL, GColorWhite);
}

/* ── Animation timer ──────────────────────────────────────────────────────── */

static void anim_timer_cb(void *context) {
    s_anim_timer = NULL;
    bool still_animating = false;
    for (int d = 0; d < 4; d++) {
        if (s_anim_rows[d] < 5) {
            s_anim_rows[d]++;
            if (s_anim_rows[d] < 5) still_animating = true;
        }
    }
    layer_mark_dirty(s_canvas);
    if (still_animating) {
        s_anim_timer = app_timer_register(ANIM_MS, anim_timer_cb, NULL);
    }
}

/* ── Tick handler ─────────────────────────────────────────────────────────── */

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    int old_digits[4] = {
        s_hours   / 10, s_hours   % 10,
        s_minutes / 10, s_minutes % 10,
    };

    s_hours   = tick_time->tm_hour;
    s_minutes = tick_time->tm_min;

    int new_digits[4] = {
        s_hours   / 10, s_hours   % 10,
        s_minutes / 10, s_minutes % 10,
    };

    /* Reset only the digits that actually changed */
    bool any_changed = false;
    for (int d = 0; d < 4; d++) {
        if (new_digits[d] != old_digits[d]) {
            s_anim_rows[d] = 0;
            any_changed = true;
        }
    }

    if (any_changed) {
        if (s_anim_timer) app_timer_cancel(s_anim_timer);
        layer_mark_dirty(s_canvas);
        s_anim_timer = app_timer_register(ANIM_MS, anim_timer_cb, NULL);
    }
}

/* ── Window handlers ──────────────────────────────────────────────────────── */

static void window_load(Window *window) {
    Layer *root   = window_get_root_layer(window);
    GRect  bounds = layer_get_bounds(root);

    s_canvas = layer_create(bounds);
    layer_set_update_proc(s_canvas, canvas_update_proc);
    layer_add_child(root, s_canvas);

    /* Show current time immediately with all rows visible */
    time_t    now = time(NULL);
    struct tm *t  = localtime(&now);
    s_hours       = t->tm_hour;
    s_minutes     = t->tm_min;
    for (int d = 0; d < 4; d++) s_anim_rows[d] = 5;  /* all rows visible on startup */
}

static void window_unload(Window *window) {
    if (s_anim_timer) {
        app_timer_cancel(s_anim_timer);
        s_anim_timer = NULL;
    }
    layer_destroy(s_canvas);
}

/* ── App lifecycle ────────────────────────────────────────────────────────── */

static void init(void) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load   = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_window, true);
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
