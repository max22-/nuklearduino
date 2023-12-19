#include <Arduino.h>
#include <TFT_eSPI.h>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include <nuklear.h>

TFT_eSPI tft;
struct nk_context ctx;
struct nk_user_font nk_font;

#define MAX_MEMORY (64*1024)

float text_width_calculation(nk_handle handle, float height, const char *text, int len) {

  //your_font_type *type = handle.ptr;

  float text_width = tft.textWidth(text);
  return text_width;
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  nk_font.userdata.ptr = nullptr;
  nk_font.height = tft.fontHeight();
  nk_font.width = text_width_calculation;
  Serial.println("Hello, World!");
  nk_init_fixed(&ctx, calloc(1, MAX_MEMORY), MAX_MEMORY, &nk_font);
}

void loop() {
  /* init gui state */



  enum {EASY, HARD};
  static int op = EASY;
  static float value = 0.6f;
  static int i =  20;

  if (nk_begin(&ctx, "Show", nk_rect(50, 50, 220, 220),
      NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
      /* fixed widget pixel width */
      nk_layout_row_static(&ctx, 30, 80, 1);
      if (nk_button_label(&ctx, "button")) {
          /* event handling */
      }

      /* fixed widget window ratio width */
      nk_layout_row_dynamic(&ctx, 30, 2);
      if (nk_option_label(&ctx, "easy", op == EASY)) op = EASY;
      if (nk_option_label(&ctx, "hard", op == HARD)) op = HARD;

      /* custom widget pixel width */
      nk_layout_row_begin(&ctx, NK_STATIC, 30, 2);
      {
          nk_layout_row_push(&ctx, 50);
          nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
          nk_layout_row_push(&ctx, 110);
          nk_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
      }
      nk_layout_row_end(&ctx);
  }
  nk_end(&ctx);

  const struct nk_command_text *tx;
  const struct nk_command_line *l;
  const struct nk_command_rect *r;
  const struct nk_command_rect_filled *rf;

  const struct nk_command *cmd = 0;
  nk_foreach(cmd, &ctx) {
    switch (cmd->type) {
    case NK_COMMAND_TEXT:
      tx = (const struct nk_command_text*)cmd;
      tft.drawString(String(tx->string, tx->length), tx->x, tx->y);
      #warning add other parameters
      break;
    case NK_COMMAND_LINE:
      l = (const struct nk_command_line*)cmd;
      tft.drawLine(l->begin.x, l->begin.y, l->end.x, l->end.y, tft.color565(l->color.r, l->color.g, l->color.b));
      #warning TODO: alpha blend ?
      break;
    case NK_COMMAND_RECT:
      r = (const struct nk_command_rect *)cmd;
      tft.drawRect(r->x, r->y, r->w, r->h, tft.color565(r->color.r, r->color.g, r->color.b));
      #warning TODO: add rounding
      break;
    case NK_COMMAND_RECT_FILLED:
      rf = (const struct nk_command_rect_filled *)cmd;
      tft.fillRect(rf->x, rf->y, rf->w, rf->h, tft.color565(rf->color.r, rf->color.g, rf->color.b));
      #warning TODO: add rounding
      break;
    }  
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