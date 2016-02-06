#include <pebble.h>

#define FACE_BACKGROUND_COLOUR GColorWhite
#define PIP_COLOUR GColorBlack
#define HOUR_COLOUR COLOR_FALLBACK(GColorDarkGreen, GColorBlack)
#define MINUTE_COLOUR COLOR_FALLBACK(GColorOxfordBlue, GColorBlack)
#define PIP_ARC_THICKNESS 5
#define PIP_ARC_LENGTH 2
#define HOUR_ARC_THICKNESS 15
#define HOUR_ARC_LENGTH 5
#define MINUTE_ARC_THICKNESS 30
#define MINUTE_ARC_LENGTH 5

#define TEMPERATURE_LAYER_HEIGHT 42
#define TEMPERATURE_LAYER_BACKGROUND GColorClear
#define TEMPERATURE_LAYER_FOREGROUND GColorBlack

#define KEY_TEMPERATURE 0

#define APPMESSAGE_BUF_SIZE APP_MESSAGE_INBOX_SIZE_MINIMUM

#define WEATHER_UPDATE_MINS 1

static Window *window;
static Layer *face_layer;
static TextLayer *temperature_layer;

#define TEMPERATURE_UNINITIALISED 65535 // Flag for uninitialised; unlikely to happen for real

int temperature = TEMPERATURE_UNINITIALISED;

char temperature_string[5];

static void face_update_proc(Layer *l, GContext *ctx) {
	GRect r = layer_get_bounds(l);
	graphics_context_set_fill_color(ctx, FACE_BACKGROUND_COLOUR);

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	// Draw the pips at quarter hours.
	graphics_context_set_fill_color(ctx, PIP_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(0 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(0 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(90 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(90 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(180 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(180 + PIP_ARC_LENGTH));
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, PIP_ARC_THICKNESS, DEG_TO_TRIGANGLE(270 - PIP_ARC_LENGTH), DEG_TO_TRIGANGLE(270 + PIP_ARC_LENGTH));
	
	// 0 <= hour < 24; 1440 minutes in a day; ((hour * 60 + min) % 720) / 720 * 360; 360 / 720 = 0.5
	int hour_degrees = (t->tm_hour * 60 + t->tm_min) % 720 / 2;
	graphics_context_set_fill_color(ctx, HOUR_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, HOUR_ARC_THICKNESS, DEG_TO_TRIGANGLE(hour_degrees - HOUR_ARC_LENGTH), DEG_TO_TRIGANGLE(hour_degrees + HOUR_ARC_LENGTH));

	// 0 <= minute < 59; 360 / 60 = 6
	int minute_degrees = t->tm_min * 6;
	graphics_context_set_fill_color(ctx, MINUTE_COLOUR);
	graphics_fill_radial(ctx, r, GOvalScaleModeFitCircle, MINUTE_ARC_THICKNESS, DEG_TO_TRIGANGLE(minute_degrees - MINUTE_ARC_LENGTH), DEG_TO_TRIGANGLE(minute_degrees + MINUTE_ARC_LENGTH));

}

static void request_weather(void) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (!iter) {
		// Error creating outbound message
		return;
	}

	int value = 1;
	dict_write_int(iter, 1, &value, sizeof(int), true);
	dict_write_end(iter);

	app_message_outbox_send();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	if((tick_time->tm_min % WEATHER_UPDATE_MINS) == 0)
		request_weather();

	layer_mark_dirty(face_layer);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	face_layer = layer_create(layer_get_frame(window_layer));
	layer_add_child(window_layer, face_layer);
	layer_set_update_proc(face_layer, face_update_proc);

	// Temperature layer is the width of the background but only one line high
	GRect tlr = GRect(0, layer_get_bounds(face_layer).size.h / 2 - TEMPERATURE_LAYER_HEIGHT / 2, layer_get_bounds(face_layer).size.w, TEMPERATURE_LAYER_HEIGHT);
	temperature_layer = text_layer_create(tlr);

	text_layer_set_background_color(temperature_layer, TEMPERATURE_LAYER_BACKGROUND);
	text_layer_set_text_color(temperature_layer, TEMPERATURE_LAYER_FOREGROUND);

	text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
	text_layer_set_text_alignment(temperature_layer, GTextAlignmentCenter);
	if(temperature == TEMPERATURE_UNINITIALISED)
		text_layer_set_text(temperature_layer, "--");

	layer_add_child(face_layer, text_layer_get_layer(temperature_layer));
}

static void window_appear(Window *window) {
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void window_disappear(Window *window) {
	tick_timer_service_unsubscribe();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage received");

	Tuple *data = dict_find(iterator, KEY_TEMPERATURE);
	if (data) {
		//APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE received with value %d", (int)data->value->int32);
		temperature = data->value->int32;
		snprintf(temperature_string, sizeof(temperature_string), "%d", temperature);
		text_layer_set_text(temperature_layer, temperature_string);
		layer_mark_dirty(face_layer);
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void init(void) {
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);

	AppMessageResult r = app_message_open(APPMESSAGE_BUF_SIZE, APPMESSAGE_BUF_SIZE);
	if(r != APP_MSG_OK)
		APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage open returned %d", r);

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.appear = window_appear,
		.load = window_load,
		.disappear = window_disappear,
	});
	window_stack_push(window, true);
}

static void deinit(void) {
	text_layer_destroy(temperature_layer);
	layer_destroy(face_layer);
	window_destroy(window);

	app_message_deregister_callbacks();
}

int main(void) {
	init();

	app_event_loop();

	deinit();
}
