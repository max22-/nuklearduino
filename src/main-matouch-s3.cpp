#include <Arduino.h>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklearduino.h"

#define MAX_MEMORY (64*1024)

#define TOUCH_SDA 39
#define TOUCH_SCL 38
#define TOUCH_INT 40
#define TOUCH_RST 1
#define TFT_BLK 45
#define TFT_RES -1

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance;
    lgfx::Touch_GT911 _touch_instance;
public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            // 16bit
            cfg.port = 0;
            cfg.freq_write = 10000000;
            cfg.pin_wr = 18;
            cfg.pin_rd = 48;
            cfg.pin_rs = 17;

            cfg.pin_d0 = 47;
            cfg.pin_d1 = 21;
            cfg.pin_d2 = 14;
            cfg.pin_d3 = 13;
            cfg.pin_d4 = 12;
            cfg.pin_d5 = 11;
            cfg.pin_d6 = 10;
            cfg.pin_d7 = 9;
            cfg.pin_d8 = 3;
            cfg.pin_d9 = 8;
            cfg.pin_d10 = 16;
            cfg.pin_d11 = 15;
            cfg.pin_d12 = 7;
            cfg.pin_d13 = 6;
            cfg.pin_d14 = 5;
            cfg.pin_d15 = 4;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs = 46;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;

            cfg.memory_width = 280;
            cfg.memory_height = 320;
            cfg.panel_width = 280;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = true;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }
        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.y_min = 0;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;

            cfg.i2c_port = I2C_NUM_1;
            cfg.pin_sda = TOUCH_SDA;
            cfg.pin_scl = TOUCH_SCL;
            cfg.pin_int = TOUCH_INT;
            cfg.pin_rst = TOUCH_RST;
            cfg.x_max = 280;
            cfg.y_max = 320;

            cfg.freq = 400000;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};



BBCapTouch bbct;
LGFX lcd;
NkBackend_LovyanGFX nk_backend(&lcd);
NkInput_BBCapTouch nk_input_bbct(bbct);

Nuklear nk(&nk_backend, &nk_input_bbct);

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  lcd.init();
  lcd.setRotation(3);
  lcd.fillScreen(TFT_BLACK);
  if(!nk.begin(MAX_MEMORY)) {
    lcd.println("Failed to initialize Nuklear");
    while(true) yield();
  }
  if(bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT) != CT_SUCCESS) {
    lcd.println("Failed to initialize touchscreen controller");
    while(true) yield();
  }
}

void loop() {
  static unsigned long fps = 0;
  struct nk_context *ctx = nk.get_context();
  nk.handle_input();
  
  enum {EASY, HARD};
  static int op = EASY;
  static float value = 0.6f;
  static int i =  20;

  
  nk_style_push_vec2(ctx, &ctx->style.window.scrollbar_size, nk_vec2(20,10));
  
  if (nk_begin(ctx, "MainWindow", nk_rect(0, 0, lcd.width(), lcd.height()), 0)) {
      /* fixed widget pixel width */
      nk_layout_row_static(ctx, 30, 80, 1);
      static bool show_button2 = false;
      if (nk_button_label(ctx, "button ")) {
          show_button2 = true;
      }

      if(show_button2) {
        if (nk_button_label(ctx, "button2")) {
          show_button2 = false;
        }
      }

      nk_label(ctx, String(ESP.getPsramSize()).c_str(), NK_TEXT_ALIGN_LEFT);
      nk_label(ctx, String(ESP.getFreePsram()).c_str(), NK_TEXT_ALIGN_LEFT);
      nk_label(ctx, (String(fps) + " FPS").c_str(), NK_TEXT_ALIGN_LEFT);

      /* fixed widget window ratio width */
      nk_layout_row_dynamic(ctx, 30, 2);
      if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

      /* custom widget pixel width */
      nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
      {
          nk_layout_row_push(ctx, 75);
          nk_label(ctx, "Volume:", NK_TEXT_LEFT);
          nk_layout_row_push(ctx, 110);
          nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
      }
      nk_layout_row_end(ctx);
  }
  nk_end(ctx);
  nk_style_pop_vec2(ctx);

  nk.render();

  static unsigned long timestamp = millis();
  static unsigned long frames = 0;
  if(millis() - timestamp > 1000) {
    timestamp = millis();
    fps = frames;
    frames = 0;
  } else frames++;
}