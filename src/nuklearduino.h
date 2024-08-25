#ifndef NUKLEARDUINO_H
#define NUKLEARDUINO_H

#include <nuklear.h>

class NkBackend {
public:
    virtual bool begin() = 0;
    virtual void begin_frame() = 0;
    virtual void clear() = 0;
    virtual void draw_text(const struct nk_command_text *) = 0;
    virtual void stroke_line(const struct nk_command_line *) = 0;
    virtual void stroke_rect(const struct nk_command_rect *) = 0;
    virtual void fill_rect(const struct nk_command_rect_filled *) = 0;
    virtual void stroke_circle(const struct nk_command_circle *c ) = 0;
    virtual void fill_circle(const struct nk_command_circle_filled *) = 0;
    virtual void scissor(const struct nk_command_scissor *) = 0;
    virtual void end_frame() = 0;
    virtual void draw_mouse(unsigned int x, unsigned int y) = 0;
    virtual void render() = 0;
    virtual void* get_font_userdata() = 0;
    virtual float (*get_text_width_calculation_func(void))(nk_handle, float, const char *, int) = 0;
    virtual float font_height() = 0;
    virtual int width() = 0;
    virtual int height() = 0;
    virtual int rotation() = 0;
};


class NkInput {
public:
    virtual int get_x() = 0;
    virtual int get_y() = 0;
protected:
    int x = 0, y = 0;
private:
    virtual void handle(struct nk_context *ctx, NkBackend *backend) = 0;
    friend class Nuklear;
};


class Nuklear {
public:
    Nuklear(NkBackend *backend, NkInput *input) : backend(backend), input(input) {}
    bool begin(size_t max_memory) {
        if(!backend->begin())
            return false;
        nk_font.userdata.ptr = backend->get_font_userdata();
        nk_font.height = backend->font_height();
        nk_font.width = backend->get_text_width_calculation_func();
        last_draw_buf = calloc(1, max_memory);
        draw_buf = calloc(1, max_memory);
        nk_init_fixed(&ctx, draw_buf, max_memory, &nk_font);
        return true;
    }

    void handle_input() {
        input->handle(&ctx, backend);
    }

    void render() {
        void *cmds = nk_buffer_memory(&ctx.memory);
        if(memcmp(cmds, last_draw_buf, ctx.memory.allocated)) {
            memcpy(last_draw_buf, cmds, ctx.memory.allocated);
            backend->clear();
            const struct nk_command *cmd = 0;
            nk_foreach(cmd, &ctx) {
                switch (cmd->type) {
                case NK_COMMAND_TEXT:
                    backend->draw_text((const struct nk_command_text*)cmd);
                    break;
                case NK_COMMAND_LINE:
                    backend->stroke_line((const struct nk_command_line*)cmd);
                    break;
                case NK_COMMAND_RECT:
                    backend->stroke_rect((const struct nk_command_rect *)cmd);
                    break;
                case NK_COMMAND_RECT_FILLED:
                    backend->fill_rect((const struct nk_command_rect_filled *)cmd);
                    break;
                case NK_COMMAND_CIRCLE:
                    backend->stroke_circle((const struct nk_command_circle *)cmd);
                    break;
                case NK_COMMAND_CIRCLE_FILLED:
                    backend->fill_circle((const struct nk_command_circle_filled *)cmd);
                    break;
                case NK_COMMAND_SCISSOR:
                    backend->scissor((const struct nk_command_scissor *)cmd);
                    break;
                }  
            }
            backend->draw_mouse(input->get_x(), input->get_y());
            backend->render();
        }
        nk_clear(&ctx);
    }

    struct nk_context *get_context() { return &ctx; }

private:
    NkBackend *backend;
    NkInput *input;
    struct nk_user_font nk_font;
    void *last_draw_buf = nullptr;
    void *draw_buf = nullptr;
    struct nk_context ctx;
};

/* TFT_eSPI Backend ********************************************************* */

#ifdef TFT_eSPI_BACKEND

#include <TFT_eSPI.h>

static float text_width_calculation(nk_handle handle, float height, const char *text, int len) {
    TFT_eSprite *spr = (TFT_eSprite*)handle.ptr;
    float text_width = spr->textWidth(text);
    return text_width;
}

class NkBackend_TFT_eSPI : public NkBackend {
public:
    NkBackend_TFT_eSPI(TFT_eSPI *tft) : tft(tft), spr(tft) {}

    bool begin() {
        spr.setTextFont(2);
        void *ptr = spr.createSprite(tft->width(), tft->height());
        return ptr != nullptr;
    }

    void begin_frame() override {}

    virtual void clear() override {
        spr.fillScreen(TFT_BLACK);
    }

    virtual void draw_text(const struct nk_command_text *text) override {
        spr.setTextColor(nk_color_to_565(text->foreground), nk_color_to_565(text->background));
        spr.drawString(String(text->string, text->length), text->x, text->y);
    }

    void stroke_line(const struct nk_command_line *line) override {
        spr.drawLine(line->begin.x, line->begin.y, line->end.x, line->end.y, nk_color_to_565(line->color));
        #warning TODO: alpha blending ?
    }

    void stroke_rect(const struct nk_command_rect *rect) override {
        spr.drawRoundRect(rect->x, rect->y, rect->w, rect->h, rect->rounding, nk_color_to_565(rect->color));
    }

    void fill_rect(const struct nk_command_rect_filled *rect) override {
        spr.fillRoundRect(rect->x, rect->y, rect->w, rect->h, rect->rounding, nk_color_to_565(rect->color));
    }

    void stroke_circle(const struct nk_command_circle *circle) override {
        int16_t rx = circle->w/2, ry = circle->h/2;
        int16_t xc = circle->x + rx, yc = circle->y + ry;
        spr.drawEllipse(xc, yc, rx, ry, nk_color_to_565(circle->color));
    }

    void fill_circle(const struct nk_command_circle_filled *circle) override {
        int16_t rx = circle->w/2, ry = circle->h/2;
        int16_t xc = circle->x + rx, yc = circle->y + ry;
        spr.fillEllipse(xc, yc, rx, ry, nk_color_to_565(circle->color));
    }

    void scissor(const struct nk_command_scissor *scissor) override {
        int32_t x = scissor->x, y = scissor->y, w = scissor->w, h = scissor->h;
        spr.setViewport(x, y, w, h, false);
    }

    void end_frame() override {}

    void draw_mouse(unsigned int x, unsigned int y) override {
        spr.fillCircle(x, y, 5, TFT_RED);
    }

    void render() override {
        spr.pushSprite(0, 0);
    }

    void* get_font_userdata() override { return &spr; }

    float (*get_text_width_calculation_func(void))(nk_handle, float, const char *, int) override {
        return text_width_calculation;
    }

    float font_height() override {
        return spr.fontHeight();
    }

    int width() override {
        return spr.width();
    }

    int height() override {
        return spr.height();
    }

    int rotation() override {
        return tft->getRotation();
    }

private:
    uint16_t nk_color_to_565(struct nk_color color) {
        return spr.color565(color.r, color.g, color.b);
    }

    TFT_eSPI *tft;
    TFT_eSprite spr;
};

#endif

/* FT6206 Touchscreen ******************************************************* */

#ifdef INPUT_FT6206

#include <Adafruit_FT6206.h>

class NkInput_FT6206 : public NkInput {
public:
    NkInput_FT6206(Adafruit_FT6206 &ctp) : ctp(ctp) {}
    int get_x() override { return x; }
    int get_y() override { return y; }
private:
    void handle(struct nk_context *ctx, NkBackend *backend) override {
        nk_input_begin(ctx);
        if(ctp.touched()) {
            TS_Point p = ctp.getPoint();
            switch(backend->rotation()) {
            case 0: // fallthrough
            default:
                x = p.x;
                y = p.y;
                break;
            case 1:
                x = p.y;
                y = backend->height() - p.x;
                break;
            case 2:
                x = backend->width() - p.x;
                y = backend->height() - p.y;
                break;
            case 3:
                x = backend->width() - p.y;
                y = p.x;
                break;
            }
            nk_input_motion(ctx, x, y);
            if(!touched) {
                nk_input_button(ctx, NK_BUTTON_LEFT, x, y, 1);
                touched = true;
            }
        } else {
            if(touched) {
                nk_input_button(ctx, NK_BUTTON_LEFT, x, y, 0);
                touched = false;
            }
        }
        nk_input_end(ctx);
    }
    Adafruit_FT6206 &ctp;
    bool touched = false;
};

#endif

#endif /* NUKLEARDUINO_H */

