#include "display.h"

#if STUB_DISPLAY > 0

void bootstrap_wire_touchpad_display() {};
void toggle_backlight() {};
bool is_backlight_on() { return true; };
void refresh_ui() {};
void set_circle_green() {};
void set_circle_red() {};

#else

#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <Wire.h>
#include "TouchDrvCSTXXX.hpp"
#include <lvgl.h>
#include "lv_conf.h"
// UI

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 10];

lv_obj_t *label;
static lv_obj_t *my_Cir;

// Touchpad

#define MAX_TOUCH_POINTS 5
int16_t x[MAX_TOUCH_POINTS];
int16_t y[MAX_TOUCH_POINTS];
TouchDrvCST92xx CST9217;
uint8_t touchAddress = 0x5A;

// Bus

Arduino_DataBus* bus = new Arduino_ESP32QSPI(
  LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
  LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);

// Display

Arduino_GFX* gfx = new Arduino_CO5300(
  bus,
  LCD_RESET /* RST */,
  0 /* rotation */,
  false /* IPS */,
  LCD_WIDTH,
  LCD_HEIGHT,
  6 /* col_offset1 */,
  0 /* row_offset1 */,
  0 /* col_offset2 */,
  0 /* row_offset2 */
);

bool backlight_on = true;


void set_display_rectangle(uint16_t side, uint16_t color) {
  gfx->fillRect(gfx->width() / 2 - side / 2, gfx->height() / 2 - side / 2, side, side, color);
}


void example_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    if(area->x1 % 2 !=0)area->x1--;
    if(area->y1 % 2 !=0)area->y1--;
    if(area->x2 %2 ==0)area->x2++;
    if(area->y2 %2 ==0)area->y2++;
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}


void example_increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static uint8_t count = 0;
void example_increase_reboot(void *arg) {
  count++;
  if (count == 30) {
    esp_restart();
  }
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  uint8_t touched = CST9217.getPoint(x, y, CST9217.getSupportTouchPoint());
  if (touched > 0) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x[0];
    data->point.y = y[0];
    toggle_backlight();

    //isSnoozed = true;

    //LOGF("Touch coordinates => X:%d Y:%d\n", x[0], y[0]);
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup_lv() {
  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.rounder_cb = example_lvgl_rounder_cb;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  //lv_obj_t *label = lv_label_create(lv_scr_act());
  //lv_label_set_text(label, "Hello Ardino and LVGL!");
  //lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);

  my_Cir = lv_obj_create(lv_scr_act());

  int size = LCD_WIDTH - 100;

  lv_obj_set_size(my_Cir , size, size);
  lv_obj_set_pos(my_Cir , LCD_WIDTH / 2 - size / 2, LCD_HEIGHT / 2 - size / 2);
  lv_obj_set_style_bg_color(my_Cir , lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
  lv_obj_set_style_outline_width(my_Cir, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_color(my_Cir, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(my_Cir , LV_RADIUS_CIRCLE, 0);


   const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  const esp_timer_create_args_t reboot_timer_args = {
    .callback = &example_increase_reboot,
    .name = "reboot"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
}

void setup_display() {
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin()) {
    //LOGLN("gfx->begin() failed!");
  }
  
}

void setup_display_default_view() {
  gfx->fillScreen(BLACK);

  int16_t center_x = gfx->width() / 2;
  int16_t center_y = gfx->height() / 2;

  set_display_rectangle(200, GREEN);
}






void setup_touchpad() {
  CST9217.begin(Wire, touchAddress, IIC_SDA, IIC_SCL);
  CST9217.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  CST9217.setMirrorXY(true, true);
}


void setup_wire() {
  Wire.begin(IIC_SDA, IIC_SCL);
}

void bootstrap_wire_touchpad_display() {
  setup_wire();
  setup_touchpad();
  setup_display();
  setup_lv();
}

void toggle_backlight() {

  if (backlight_on) {
    for (int i = 255; i >= 0; i--) {
      gfx->Display_Brightness(i);
      delay(3);
    }
  } else {
    for (int i = 0; i <= 255; i++) {
      gfx->Display_Brightness(i);
      delay(3);
    }
  }
  backlight_on = !backlight_on;
}

bool is_backlight_on() {
  return backlight_on;
}

void refresh_ui() {
  lv_timer_handler();
}

void set_circle_color(lv_palette_t color) {
  lv_obj_set_style_bg_color(my_Cir , lv_palette_main(color), LV_PART_MAIN);
  lv_obj_set_style_outline_width(my_Cir, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_color(my_Cir, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_timer_handler();
}

void set_circle_green() {
  set_circle_color(LV_PALETTE_GREEN);
}

void set_circle_red() {
  set_circle_color(LV_PALETTE_RED);
}

#endif
