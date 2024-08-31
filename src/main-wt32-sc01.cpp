#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklearduino.h"

#define MAX_MEMORY (64*1024)

TFT_eSPI tft;
NkBackend_TFT_eSPI backend(&tft);

Adafruit_FT6206 ctp = Adafruit_FT6206();
NkInput_FT6206 ft6206(ctp);

Nuklear nk(&backend, &ft6206);
struct nk_context *ctx = nullptr;

void setup() {
  Serial.begin(115200);
  tft.init();
  //tft.setRotation(3);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  if(!nk.begin(MAX_MEMORY)) {
    tft.println("Failed to initialize Nuklear");
    while(true) yield();
  }

  Wire.begin(18, 19);
  if (!ctp.begin(5)) {
    tft.println("Failed to start FT6206 touchscreen controller");
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
  
  if (nk_begin(ctx, "MainWindow", nk_rect(0, 0, tft.width(), tft.height()), 0)) {
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