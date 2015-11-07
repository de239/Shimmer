#include <pebble.h>

#define FACE_BACKGROUND_COLOUR GColorBlack
#define PIP_COLOUR GColorWhite
#define HOUR_COLOUR GColorDarkGreen
#define MINUTE_COLOUR GColorBrightGreen
#define PIP_ARC_THICKNESS 2
#define PIP_ARC_LENGTH 1
#define HOUR_ARC_THICKNESS 3
#define HOUR_ARC_LENGTH 5
#define MINUTE_ARC_THICKNESS 5
#define MINUTE_ARC_LENGTH 5

static Window *window;
static Layer *face_layer;

static void face_update_proc(Layer *l, GContext *ctx) {
	GRect r = layer_get_bounds(l);
	graphics_context_set_fill_color(ctx, FACE_BACKGROUND_COLOUR);
	graphics_fill_rect(ctx, r, 0, GCornerNone);

	time_t now = time(NULL);
	struct tm t = *localtime(&now);

	// Draw the pips at quarter hours.
	graphics_context_set_fill_color(ctx, PIP_COLOUR);
	graphics_context_set_stroke_color(ctx, PIP_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(0 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(0 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(90 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(90 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(180 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(180 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(270 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(270 + PIP_ARC_LENGTH));
	
	// 0 <= hour < 24; 1440 minutes in a day; ((hour * 60 + min) % 720) / 720 * 360; 360 / 720 = 0.5
	int hour_degrees = (t.tm_hour * 60 + t.tm_min) % 720 / 2;
	graphics_context_set_fill_color(ctx, HOUR_COLOUR);
	graphics_context_set_stroke_color(ctx, HOUR_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, HOUR_ARC_THICKNESS, DEG_TO_TRIGANGLE(hour_degrees - HOUR_ARC_LENGTH), DEG_TO_TRIGANGLE(hour_degrees + HOUR_ARC_LENGTH));

	// 0 <= minute < 59; 360 / 60 = 6
	int minute_degrees = t.tm_min * 6;
	graphics_context_set_fill_color(ctx, MINUTE_COLOUR);
	graphics_context_set_stroke_color(ctx, MINUTE_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, MINUTE_ARC_THICKNESS, DEG_TO_TRIGANGLE(minute_degrees - MINUTE_ARC_LENGTH), DEG_TO_TRIGANGLE(minute_degrees + MINUTE_ARC_LENGTH));
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(face_layer);
}

static void window_appear(Window *window) {
	face_layer = window_get_root_layer(window);
	layer_set_update_proc(face_layer, face_update_proc);

	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void window_disappear(Window *window) {
	tick_timer_service_unsubscribe();
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.appear = window_appear,
		.disappear = window_disappear,
	});
	window_stack_push(window, true);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();

	app_event_loop();

	deinit();
}
