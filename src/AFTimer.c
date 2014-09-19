#include <pebble.h>

static Window *window;
static TextLayer *chooseTimerLabel;
static Layer *timerProgressLayer;
static int timerAmount;
static int timerProgress;
static bool minuteMode;
static bool timerActivated;
static AppTimer *progressTimer;
static const uint32_t const segments[] = { 100, 500, 100, 500, 100 };

static void setTimerAmountText(void) {
  int minuteAmount = timerAmount / 60;
  char *minuteAmountPadding = (minuteAmount < 10) ? "0" : "";
  int remainingSeconds = timerAmount - minuteAmount * 60;
  char *remainingSecondsPadding = (remainingSeconds < 10) ? "0" : "";
  static char newTimerTextBuffer[8];
  snprintf(newTimerTextBuffer, sizeof(newTimerTextBuffer), "%s%d : %s%d", minuteAmountPadding, minuteAmount, remainingSecondsPadding, remainingSeconds);
  text_layer_set_text(chooseTimerLabel, newTimerTextBuffer);

  remainingSecondsPadding = NULL;
  minuteAmountPadding = NULL;
}

static void resetTimer(void) {
  progressTimer = NULL;
  minuteMode = timerActivated = false;
  timerAmount = 0;
  layer_remove_from_parent(timerProgressLayer);
  layer_destroy(timerProgressLayer);
  timerProgressLayer = NULL;
  layer_set_hidden(text_layer_get_layer(chooseTimerLabel), false);
  text_layer_set_text(chooseTimerLabel, "00 : 00");
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (timerActivated) {
    resetTimer();
  } else {
    minuteMode = !minuteMode;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (timerActivated) {return;}
  if (minuteMode) {timerAmount += 60;} else {timerAmount++;}
  if (timerAmount >= 3600) {timerAmount = 3600;}
  setTimerAmountText();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (timerActivated) {return;}
  if (minuteMode) {timerAmount -= 60;} else {timerAmount--;}
  if (timerAmount < 0) {timerAmount = 0;}
  setTimerAmountText();
}

static void timerComplete() {
  if (!timerActivated) {
    return;
  }
  resetTimer();
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
}

static void updateTimerProgressData(void *data) {
  if (timerProgress >= timerAmount) {timerComplete(); return;}
  timerProgress++;
  progressTimer = NULL;
  progressTimer = app_timer_register(1000, updateTimerProgressData, NULL);
  layer_mark_dirty(timerProgressLayer);
}

static void updateTimerProgressVisuals(struct Layer *layer, GContext *ctx) {
  Layer *window_layer = window_get_root_layer(window);
  GRect mainBounds = layer_get_bounds(window_layer);
  float progress_percent = timerProgress / (float)timerAmount;
  progress_percent = progress_percent * (float)100.0;
  progress_percent = progress_percent * mainBounds.size.h / (float)100.0;
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, (progress_percent <= 0 ) ? progress_percent : progress_percent+1}, .size = { mainBounds.size.w, mainBounds.size.h - progress_percent} }, 0, GCornerNone);
  window_layer = NULL;
}

static void select_click_handler_start_timer(ClickRecognizerRef recognizer, void *context) {
  if (timerActivated || timerAmount <= 0) {return;}
  timerActivated = true;
  timerProgress = 0;
  progressTimer = NULL;
  progressTimer = app_timer_register(1000, updateTimerProgressData, NULL);
  Layer *window_layer = window_get_root_layer(window);
  timerProgressLayer = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(timerProgressLayer, updateTimerProgressVisuals);
  layer_set_hidden(text_layer_get_layer(chooseTimerLabel), true);
  layer_add_child(window_layer, timerProgressLayer);
  window_layer = NULL;
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_click_handler_start_timer, NULL);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  timerAmount = 0;
  minuteMode = timerActivated = false;
  chooseTimerLabel = text_layer_create((GRect) { .origin = { 0, 68 }, .size = { bounds.size.w, 28 } });
  text_layer_set_text(chooseTimerLabel, "00 : 00");
  text_layer_set_font(chooseTimerLabel, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(chooseTimerLabel, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(chooseTimerLabel));
  window_layer = NULL;
}

static void window_unload(Window *window) {

}

static void init(void) {
  window = window_create();
  window_set_fullscreen(window, true);
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);
  window_layer = NULL;

  if (progressTimer != NULL) {
    app_timer_cancel(progressTimer);
    progressTimer = NULL;
  }

  if (timerProgressLayer != NULL) {
    layer_set_update_proc(timerProgressLayer, NULL);
    layer_destroy(timerProgressLayer);
    timerProgressLayer = NULL;
  }

  text_layer_destroy(chooseTimerLabel);
  chooseTimerLabel = NULL;

  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}