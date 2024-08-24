#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include <nuklear.h>

#define MAX_MEMORY (64*1024)

TFT_eSPI tft;
TFT_eSprite spr(&tft);
Adafruit_FT6206 ctp = Adafruit_FT6206();
struct nk_context ctx;
struct nk_user_font nk_font;
void *last_draw_buf;
void *draw_buf;

float text_width_calculation(nk_handle handle, float height, const char *text, int len) {
  //your_font_type *type = handle.ptr;
  float text_width = spr.textWidth(text);
  return text_width;
}

static inline uint16_t nk_color_to_565(nk_color color) {
  return tft.color565(color.r, color.g, color.b);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  if(!spr.createSprite(tft.width(), tft.height()))
    Serial.println("Failed to create sprite");
  //spr.setTextSize(2);
  //spr.setFreeFont(&FreeSerif9pt7b);
  spr.setTextFont(2);
  Wire.begin(18, 19);
  if (!ctp.begin(5)) {
    Serial.println("Couldn't start FT6206 touchscreen controller");
    while(true) yield();
  }
  nk_font.userdata.ptr = nullptr;
  nk_font.height = spr.fontHeight();
  nk_font.width = text_width_calculation;
  Serial.println("Hello, World!");
  last_draw_buf = calloc(1, MAX_MEMORY);
  draw_buf = calloc(1, MAX_MEMORY);
  nk_init_fixed(&ctx, draw_buf, MAX_MEMORY, &nk_font);
}

void loop() {

  nk_input_begin(&ctx);
  static bool touched = false;
  static int16_t x = 0, y = 0;

  if(ctp.touched()) {
    TS_Point p = ctp.getPoint();
    x = tft.width() - p.y;
    y = p.x;
    Serial.printf("x = %d\ty=%d\n", x, y);
    nk_input_motion(&ctx, x, y);
    if(!touched)
      nk_input_button(&ctx, NK_BUTTON_LEFT, x, y, 1);
    touched = true;
    //spr.drawPixel(x, y, TFT_RED);
  } else {
    if(touched)
      nk_input_button(&ctx, NK_BUTTON_LEFT, x, y, 0);
    touched = false;
  }

  nk_input_end(&ctx);

  /* init gui state */

  enum {EASY, HARD};
  static int op = EASY;
  static float value = 0.6f;
  static int i =  20;

  if (nk_begin(&ctx, "Show", nk_rect(50, 50, 220, 220),
      NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
      /* fixed widget pixel width */
      nk_layout_row_static(&ctx, 30, 80, 1);
      static bool show_button2 = false;
      if (nk_button_label(&ctx, "button ")) {
          show_button2 = true;
      }

      if(show_button2) {
        if (nk_button_label(&ctx, "button2")) {
          show_button2 = false;
        }
      }

      nk_label(&ctx, String(ESP.getPsramSize()).c_str(), NK_TEXT_ALIGN_LEFT);
      nk_label(&ctx, String(ESP.getFreePsram()).c_str(), NK_TEXT_ALIGN_LEFT);

      /* fixed widget window ratio width */
      nk_layout_row_dynamic(&ctx, 30, 2);
      if (nk_option_label(&ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(&ctx, "hard", op == HARD)) op = HARD;

      /* custom widget pixel width */
      nk_layout_row_begin(&ctx, NK_STATIC, 30, 2);
      {
          nk_layout_row_push(&ctx, 75);
          nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
          nk_layout_row_push(&ctx, 110);
          nk_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
      }
      nk_layout_row_end(&ctx);
  }
  nk_end(&ctx);

  void *cmds = nk_buffer_memory(&ctx.memory);
  if(memcmp(cmds, last_draw_buf, ctx.memory.allocated)) {
    memcpy(last_draw_buf, cmds, ctx.memory.allocated);
    spr.fillSprite(TFT_BLACK);
    const struct nk_command *cmd = 0;
    nk_foreach(cmd, &ctx) {
      switch (cmd->type) {
      case NK_COMMAND_TEXT: {
        const struct nk_command_text *tx = (const struct nk_command_text*)cmd;
        spr.setTextColor(nk_color_to_565(tx->foreground), nk_color_to_565(tx->background));
        spr.drawString(String(tx->string, tx->length), tx->x, tx->y);
        break;
      }
      case NK_COMMAND_LINE: {
        const struct nk_command_line *l = (const struct nk_command_line*)cmd;
        spr.drawLine(l->begin.x, l->begin.y, l->end.x, l->end.y, spr.color565(l->color.r, l->color.g, l->color.b));
        #warning TODO: alpha blend ?
        break;
      }
      case NK_COMMAND_RECT: {
        const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
        spr.drawRoundRect(r->x, r->y, r->w, r->h, r->rounding, spr.color565(r->color.r, r->color.g, r->color.b));
        break;
      }
      case NK_COMMAND_RECT_FILLED: {
        const struct nk_command_rect_filled *rf = (const struct nk_command_rect_filled *)cmd;
        spr.fillRoundRect(rf->x, rf->y, rf->w, rf->h, rf->rounding, spr.color565(rf->color.r, rf->color.g, rf->color.b));
        break;
      }
      case NK_COMMAND_CIRCLE: {
        const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
        int16_t rx = c->w/2, ry = c->h/2;
        int16_t xc = c->x + rx, yc = c->y + ry;
        #warning TODO: is it well centered ?
        spr.drawEllipse(xc, yc, rx, ry, nk_color_to_565(c->color));
        break;
      }
      case NK_COMMAND_CIRCLE_FILLED: {
        const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
        int16_t rx = c->w/2, ry = c->h/2;
        int16_t xc = c->x + rx, yc = c->y + ry;
        #warning TODO: is it well centered ?
        spr.fillEllipse(xc, yc, rx, ry, nk_color_to_565(c->color));
        break;
      }
      case NK_COMMAND_SCISSOR: {
        const struct nk_command_scissor *c = (const struct nk_command_scissor *)cmd;
        int32_t x = c->x, y = c->y, w = c->w, h = c->h;
        spr.setViewport(x, y, w, h, false);
        break;
      }
      }  
    }
    spr.pushSprite(0, 0);
  }
  nk_clear(&ctx);

  static unsigned long timestamp = millis();
  static unsigned long fps = 0;
  if(millis() - timestamp > 1000) {
    timestamp = millis();
    tft.drawString(String(fps) + " FPS", 0, 0);
    fps = 0;
  } else fps++;
}