#include "app.h"
#include "library.h"
#include "video.h"
#include "text.h"
#include "launcher.h"
#include "util.h"

#include <SDL.h>
#include <SDL_image.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define BASE_W 1280
#define BASE_H 720
#define VIDEO_DELAY_MS 400
#define HOVER_GRACE_MS 260

typedef struct App {
    SDL_Window *win;
    SDL_Renderer *ren;
    Library *lib;
    SDL_Texture **icons;
    unsigned char *icon_tried;
    int *icons_w;
    int *icons_h;
    SDL_Texture **covers;
    unsigned char *cover_tried;
    int *covers_w;
    int *covers_h;
    SDL_Texture *def_icon;
    int def_w;
    int def_h;
    int theme;
    Fonts *fonts;
    int selected;
    int running;
    Uint32 hover_start;
    Uint32 leave_time;
    int mouse_active;
    int pinned;
    Uint32 frame_last;
    double frame_dt;
    double scroll;
    Video *video;
    int video_playing;
    int mouse_x;
    int mouse_y;
    char status[256];
    Uint32 status_until;

    double scale;
    int tile_w;
    int tile_h;
    int tile_gap;
    int grid_pad;
    int icon_box;
    int title_h;
    int pad;
    int radius;
} App;

/*
 * Temas visuales. Estetica "ciber" de los 2000: marcos con degradado, brillos
 * y acentos neon. El usuario cambia de tema con el boton de la esquina.
 */
#define RGB(R, G, B) { (Uint8)(R), (Uint8)(G), (Uint8)(B), 255 }

typedef struct {
    const char *name;
    SDL_Color bg, bg_top, bg_bot;
    SDL_Color blue, blue_lt, frame, blue2;
    SDL_Color text, dim, key, white;
    SDL_Color panel_edge, panel_top, panel_bot;
    SDL_Color hdr_l, hdr_m, hdr_r;
    SDL_Color tile, tile_edge, tile_sel;
    SDL_Color title_l, title_m, title_r;
    SDL_Color btn_l, btn_m, btn_d, btn_hov_l, btn_hov_m;
    SDL_Color hero_bg, row_line;
    SDL_Color ph_bg, ph_ring, ph_fg;
    SDL_Color tile_ph, tile_ph_fg, tile_ph_key;
    SDL_Color sel_text;
} Theme;

static const Theme THEMES[] = {
    { "Cyber 2000",
      RGB(10, 14, 26), RGB(28, 40, 70), RGB(8, 12, 22),
      RGB(0, 150, 190), RGB(0, 200, 230), RGB(0, 150, 190), RGB(0, 190, 220),
      RGB(206, 224, 238), RGB(120, 150, 172), RGB(0, 200, 225), RGB(240, 255, 255),
      RGB(0, 140, 175), RGB(20, 32, 56), RGB(10, 16, 30),
      RGB(0, 60, 90), RGB(0, 130, 165), RGB(0, 90, 120),
      RGB(20, 32, 54), RGB(46, 84, 116), RGB(26, 64, 92),
      RGB(16, 36, 72), RGB(28, 84, 140), RGB(70, 150, 200),
      RGB(70, 225, 235), RGB(0, 150, 180), RGB(0, 80, 110), RGB(150, 250, 255), RGB(0, 190, 215),
      RGB(0, 0, 0), RGB(32, 54, 84),
      RGB(18, 28, 48), RGB(0, 140, 175), RGB(110, 160, 185),
      RGB(26, 42, 68), RGB(96, 132, 156), RGB(110, 160, 185),
      RGB(120, 235, 250) },
    { "Luna XP",
      RGB(255, 255, 255), RGB(255, 255, 255), RGB(214, 228, 247),
      RGB(0, 84, 200), RGB(96, 170, 245), RGB(0, 84, 200), RGB(0, 102, 204),
      RGB(12, 24, 48), RGB(108, 118, 138), RGB(0, 66, 128), RGB(255, 255, 255),
      RGB(0, 84, 200), RGB(255, 255, 255), RGB(214, 228, 247),
      RGB(0, 84, 227), RGB(26, 108, 240), RGB(74, 158, 255),
      RGB(255, 255, 255), RGB(166, 191, 226), RGB(207, 223, 246),
      RGB(0, 84, 227), RGB(26, 108, 240), RGB(74, 158, 255),
      RGB(152, 212, 92), RGB(74, 160, 40), RGB(44, 110, 20), RGB(198, 238, 140), RGB(104, 186, 56),
      RGB(255, 255, 255), RGB(200, 214, 235),
      RGB(255, 255, 255), RGB(140, 170, 210), RGB(120, 140, 170),
      RGB(223, 231, 245), RGB(108, 118, 138), RGB(120, 140, 170),
      RGB(0, 84, 200) },
    { "Matrix",
      RGB(0, 4, 0), RGB(0, 16, 2), RGB(0, 6, 0),
      RGB(0, 180, 40), RGB(120, 255, 140), RGB(0, 170, 40), RGB(0, 200, 50),
      RGB(130, 255, 150), RGB(60, 150, 72), RGB(0, 230, 70), RGB(200, 255, 205),
      RGB(0, 150, 40), RGB(0, 16, 2), RGB(0, 6, 0),
      RGB(0, 40, 0), RGB(0, 90, 10), RGB(0, 130, 20),
      RGB(0, 14, 2), RGB(0, 80, 20), RGB(0, 40, 8),
      RGB(0, 30, 0), RGB(0, 70, 6), RGB(0, 120, 18),
      RGB(70, 230, 90), RGB(0, 150, 30), RGB(0, 90, 18), RGB(150, 255, 160), RGB(0, 190, 40),
      RGB(0, 0, 0), RGB(0, 60, 0),
      RGB(0, 16, 2), RGB(0, 150, 40), RGB(90, 200, 110),
      RGB(0, 20, 4), RGB(60, 150, 72), RGB(90, 200, 110),
      RGB(150, 255, 160) },
    { "Chrome",
      RGB(228, 232, 238), RGB(236, 239, 244), RGB(198, 206, 220),
      RGB(70, 110, 170), RGB(120, 160, 215), RGB(90, 110, 150), RGB(90, 130, 190),
      RGB(30, 40, 60), RGB(110, 120, 140), RGB(50, 80, 140), RGB(255, 255, 255),
      RGB(150, 170, 200), RGB(238, 242, 248), RGB(204, 213, 227),
      RGB(120, 140, 175), RGB(160, 180, 208), RGB(205, 220, 240),
      RGB(246, 248, 251), RGB(170, 185, 210), RGB(205, 220, 240),
      RGB(110, 130, 168), RGB(150, 172, 202), RGB(200, 215, 235),
      RGB(150, 180, 225), RGB(70, 110, 175), RGB(40, 70, 125), RGB(190, 212, 240), RGB(100, 145, 205),
      RGB(255, 255, 255), RGB(206, 214, 228),
      RGB(246, 248, 251), RGB(150, 170, 200), RGB(120, 130, 150),
      RGB(228, 233, 240), RGB(120, 130, 150), RGB(150, 170, 200),
      RGB(40, 70, 140) },
    { "Neon",
      RGB(22, 4, 32), RGB(40, 10, 58), RGB(14, 2, 22),
      RGB(200, 60, 255), RGB(230, 120, 255), RGB(200, 60, 255), RGB(255, 70, 220),
      RGB(240, 208, 250), RGB(172, 132, 192), RGB(255, 100, 230), RGB(255, 255, 255),
      RGB(150, 44, 200), RGB(34, 8, 50), RGB(18, 4, 28),
      RGB(70, 10, 110), RGB(130, 30, 180), RGB(90, 15, 140),
      RGB(32, 10, 48), RGB(96, 42, 138), RGB(58, 18, 86),
      RGB(48, 8, 80), RGB(110, 24, 160), RGB(180, 60, 230),
      RGB(255, 120, 245), RGB(170, 40, 205), RGB(110, 20, 150), RGB(255, 170, 250), RGB(200, 70, 235),
      RGB(0, 0, 0), RGB(62, 26, 92),
      RGB(30, 8, 46), RGB(150, 44, 200), RGB(180, 130, 205),
      RGB(36, 12, 54), RGB(160, 110, 185), RGB(180, 130, 205),
      RGB(255, 150, 245) },
};
#define THEME_COUNT ((int)(sizeof(THEMES) / sizeof(THEMES[0])))

static Theme CUR;

#define BG (CUR.bg)
#define BG_TOP (CUR.bg_top)
#define BG_BOT (CUR.bg_bot)
#define XP_BLUE (CUR.blue)
#define XP_BLUE_LT (CUR.blue_lt)
#define XP_FRAME (CUR.frame)
#define XP_BLUE2 (CUR.blue2)
#define COL_TEXT (CUR.text)
#define COL_DIM (CUR.dim)
#define COL_KEY (CUR.key)
#define COL_WHITE (CUR.white)
#define COL_PANEL_EDGE (CUR.panel_edge)
#define COL_PANEL_TOP (CUR.panel_top)
#define COL_PANEL_BOT (CUR.panel_bot)
#define COL_HDR_L (CUR.hdr_l)
#define COL_HDR_M (CUR.hdr_m)
#define COL_HDR_R (CUR.hdr_r)
#define COL_TILE (CUR.tile)
#define COL_TILE_EDGE (CUR.tile_edge)
#define COL_TILE_SEL (CUR.tile_sel)
#define TITLE_L (CUR.title_l)
#define TITLE_M (CUR.title_m)
#define TITLE_R (CUR.title_r)
#define BTN_L (CUR.btn_l)
#define BTN_M (CUR.btn_m)
#define BTN_D (CUR.btn_d)
#define BTN_HOV_L (CUR.btn_hov_l)
#define BTN_HOV_M (CUR.btn_hov_m)
#define COL_HERO_BG (CUR.hero_bg)
#define COL_ROW_LINE (CUR.row_line)
#define COL_PH_BG (CUR.ph_bg)
#define COL_PH_RING (CUR.ph_ring)
#define COL_PH_FG (CUR.ph_fg)
#define COL_TILE_PH (CUR.tile_ph)
#define COL_TILE_PH_FG (CUR.tile_ph_fg)
#define COL_TILE_PH_KEY (CUR.tile_ph_key)
#define COL_SEL_TEXT (CUR.sel_text)

static void theme_apply(int i)
{
    if (i < 0 || i >= THEME_COUNT)
        i = 0;
    CUR = THEMES[i];
}

static void round_rect(SDL_Renderer *ren, int x, int y, int w, int h, int r)
{
    if (w <= 0 || h <= 0)
        return;
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;
    if (r < 0)
        r = 0;
    for (int i = 0; i < h; i++) {
        int dx = 0;
        if (i < r) {
            int dy = r - i;
            dx = r - (int)(sqrt((double)(r * r - dy * dy)) + 0.5);
        } else if (i >= h - r) {
            int dy = i - (h - 1 - r);
            dx = r - (int)(sqrt((double)(r * r - dy * dy)) + 0.5);
        }
        SDL_RenderDrawLine(ren, x + dx, y + i, x + w - 1 - dx, y + i);
    }
}

static void round_rect_c(SDL_Renderer *ren, SDL_Rect r, int rad, SDL_Color c)
{
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    round_rect(ren, r.x, r.y, r.w, r.h, rad);
}

static void fill_circle(SDL_Renderer *ren, int cx, int cy, int r, SDL_Color c)
{
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)(sqrt((double)(r * r - dy * dy)) + 0.5);
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void ring(SDL_Renderer *ren, int cx, int cy, int r, SDL_Color c)
{
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        SDL_RenderDrawPoint(ren, cx + x, cy + y);
        SDL_RenderDrawPoint(ren, cx + y, cy + x);
        SDL_RenderDrawPoint(ren, cx - y, cy + x);
        SDL_RenderDrawPoint(ren, cx - x, cy + y);
        SDL_RenderDrawPoint(ren, cx - x, cy - y);
        SDL_RenderDrawPoint(ren, cx - y, cy - x);
        SDL_RenderDrawPoint(ren, cx + y, cy - x);
        SDL_RenderDrawPoint(ren, cx + x, cy - y);
        y++;
        if (err < 0)
            err += 2 * y + 1;
        else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

static void draw_palette_icon(SDL_Renderer *ren, int cx, int cy, int R, SDL_Color body,
                              SDL_Color hole, const SDL_Color *dots, int ndots)
{
    if (R < 4)
        R = 4;
    fill_circle(ren, cx, cy, R, body);
    ring(ren, cx, cy, R, (SDL_Color){ 15, 18, 26, 170 });
    int hr = (int)(R * 0.30);
    if (hr < 2)
        hr = 2;
    fill_circle(ren, cx - (int)(R * 0.44), cy + (int)(R * 0.44), hr, hole);
    int dr = (int)(R * 0.17);
    if (dr < 2)
        dr = 2;
    static const int px[4] = { -56, -21, 25, 56 };
    static const int py[4] = { -30, -56, -52, -14 };
    for (int i = 0; i < ndots && i < 4; i++)
        fill_circle(ren, cx + px[i] * R / 100, cy + py[i] * R / 100, dr, dots[i]);
}

static SDL_Color mix(SDL_Color a, SDL_Color b, double t)
{
    SDL_Color c;
    c.a = 255;
    c.r = (Uint8)(a.r + (b.r - a.r) * t + 0.5);
    c.g = (Uint8)(a.g + (b.g - a.g) * t + 0.5);
    c.b = (Uint8)(a.b + (b.b - a.b) * t + 0.5);
    return c;
}

static void vgrad(SDL_Renderer *ren, SDL_Rect r, SDL_Color top, SDL_Color bot)
{
    if (r.w <= 0 || r.h <= 0)
        return;
    for (int i = 0; i < r.h; i++) {
        double t = (double)i / (double)(r.h > 1 ? r.h - 1 : 1);
        SDL_Color c = mix(top, bot, t);
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(ren, r.x, r.y + i, r.x + r.w - 1, r.y + i);
    }
}

static void vgrad_round(SDL_Renderer *ren, SDL_Rect r, int rad, SDL_Color top, SDL_Color bot)
{
    if (r.w <= 0 || r.h <= 0)
        return;
    if (rad > r.w / 2)
        rad = r.w / 2;
    if (rad > r.h / 2)
        rad = r.h / 2;
    if (rad < 0)
        rad = 0;
    for (int i = 0; i < r.h; i++) {
        int dx = 0;
        if (i < rad) {
            int dy = rad - i;
            dx = rad - (int)(sqrt((double)(rad * rad - dy * dy)) + 0.5);
        } else if (i >= r.h - rad) {
            int dy = i - (r.h - 1 - rad);
            dx = rad - (int)(sqrt((double)(rad * rad - dy * dy)) + 0.5);
        }
        double t = (double)i / (double)(r.h > 1 ? r.h - 1 : 1);
        SDL_Color c = mix(top, bot, t);
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(ren, r.x + dx, r.y + i, r.x + r.w - 1 - dx, r.y + i);
    }
}

static void hgrad3(SDL_Renderer *ren, SDL_Rect r, SDL_Color l, SDL_Color m, SDL_Color rr)
{
    if (r.w <= 0 || r.h <= 0)
        return;
    for (int x = 0; x < r.w; x++) {
        double t = (double)x / (double)(r.w > 1 ? r.w - 1 : 1);
        SDL_Color c = t < 0.5 ? mix(l, m, t * 2.0) : mix(m, rr, (t - 0.5) * 2.0);
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(ren, r.x + x, r.y, r.x + x, r.y + r.h - 1);
    }
}

static void gloss_strip(SDL_Renderer *ren, SDL_Rect r, int rad, int height, Uint8 a)
{
    if (r.w <= 0 || height <= 0)
        return;
    SDL_BlendMode bm;
    SDL_GetRenderDrawBlendMode(ren, &bm);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    if (height > r.h)
        height = r.h;
    if (rad > r.w / 2)
        rad = r.w / 2;
    for (int i = 0; i < height; i++) {
        int dx = 0;
        if (i < rad) {
            int dy = rad - i;
            dx = rad - (int)(sqrt((double)(rad * rad - dy * dy)) + 0.5);
        }
        Uint8 al = (Uint8)(a * (1.0 - (double)i / (double)(height > 1 ? height : 1)));
        SDL_SetRenderDrawColor(ren, 255, 255, 255, al);
        SDL_RenderDrawLine(ren, r.x + dx, r.y + i, r.x + r.w - 1 - dx, r.y + i);
    }
    SDL_SetRenderDrawBlendMode(ren, bm);
}

static SDL_Rect fit_into(int iw, int ih, SDL_Rect box)
{
    SDL_Rect r = { box.x, box.y, 0, 0 };
    if (iw <= 0 || ih <= 0)
        return r;
    double sx = (double)box.w / (double)iw;
    double sy = (double)box.h / (double)ih;
    double s = sx < sy ? sx : sy;
    r.w = (int)(iw * s);
    r.h = (int)(ih * s);
    r.x = box.x + (box.w - r.w) / 2;
    r.y = box.y + (box.h - r.h) / 2;
    return r;
}

static void app_metrics(App *a)
{
    int w = BASE_W, h = BASE_H;
    SDL_GetRendererOutputSize(a->ren, &w, &h);

    double s = (double)h / (double)BASE_H;
    if (s < 1.0)
        s = 1.0;
    if (s > 2.5)
        s = 2.5;

    a->scale = s;
    a->tile_w = (int)(132 * s);
    a->tile_h = (int)(156 * s);
    a->tile_gap = (int)(16 * s);
    a->grid_pad = (int)(20 * s);
    a->icon_box = (int)(96 * s);
    a->title_h = (int)(76 * s);
    a->pad = (int)(18 * s);
    a->radius = (int)(4 * s);
}

static void app_status(App *a, const char *text)
{
    strncpy(a->status, text, sizeof(a->status) - 1);
    a->status[sizeof(a->status) - 1] = 0;
    a->status_until = SDL_GetTicks() + 3500;
}

static SDL_Texture *create_default_icon(App *a)
{
    SDL_Renderer *ren = a->ren;
    Fonts *fn = a->fonts;
    int size = a->icon_box;
    if (size < 64)
        size = 64;
    SDL_Texture *t = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, size, size);
    if (!t)
        return NULL;
    if (SDL_SetRenderTarget(ren, t) != 0) {
        SDL_DestroyTexture(t);
        return NULL;
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
    SDL_RenderClear(ren);

    int m = size / 6;
    SDL_Rect r = { m, m, size - m * 2, size - m * 2 };
    int rad = size / 8;
    round_rect_c(ren, r, rad, XP_FRAME);
    SDL_Rect inner = { r.x + 2, r.y + 2, r.w - 4, r.h - 4 };
    vgrad_round(ren, inner, rad > 2 ? rad - 2 : 0, XP_BLUE_LT, XP_BLUE2);
    gloss_strip(ren, inner, rad > 2 ? rad - 2 : 0, inner.h / 2, 100);
    int cx = size / 2;
    int cy = size / 2;
    int cr = size / 5;
    fill_circle(ren, cx, cy, cr, COL_WHITE);
    ring(ren, cx, cy, cr, COL_KEY);
    if (fn && fn->title) {
        int tw = text_measure(fn->title, "?");
        int th = TTF_FontHeight(fn->title);
        text_draw(ren, fn->title, "?", COL_KEY, cx - tw / 2, cy - th / 2);
    }

    SDL_SetRenderTarget(ren, NULL);
    SDL_QueryTexture(t, NULL, NULL, &a->def_w, &a->def_h);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}

static SDL_Texture *app_icon(App *a, int i)
{
    if (i < 0 || i >= a->lib->count)
        return NULL;
    if (a->icon_tried[i]) {
        if (a->icons[i])
            return a->icons[i];
        if (a->def_icon) {
            a->icons_w[i] = a->def_w;
            a->icons_h[i] = a->def_h;
        }
        return a->def_icon;
    }
    a->icon_tried[i] = 1;
    Game *g = &a->lib->games[i];
    if (g->icon) {
        SDL_Texture *t = IMG_LoadTexture(a->ren, g->icon);
        if (t) {
            a->icons[i] = t;
            SDL_QueryTexture(t, NULL, NULL, &a->icons_w[i], &a->icons_h[i]);
            return t;
        }
    }
    if (a->def_icon) {
        a->icons_w[i] = a->def_w;
        a->icons_h[i] = a->def_h;
    }
    return a->def_icon;
}

static SDL_Texture *app_cover(App *a, int i, int *w, int *h)
{
    if (i < 0 || i >= a->lib->count)
        return NULL;
    if (a->cover_tried[i]) {
        if (a->covers[i]) {
            *w = a->covers_w[i];
            *h = a->covers_h[i];
        }
        return a->covers[i];
    }
    a->cover_tried[i] = 1;
    Game *g = &a->lib->games[i];
    if (g->cover) {
        SDL_Texture *t = IMG_LoadTexture(a->ren, g->cover);
        if (t) {
            a->covers[i] = t;
            SDL_QueryTexture(t, NULL, NULL, &a->covers_w[i], &a->covers_h[i]);
            *w = a->covers_w[i];
            *h = a->covers_h[i];
            return t;
        }
    }
    return NULL;
}

static SDL_Rect panel_rect(App *a, int w, int h)
{
    double s = a->scale;
    int pw = w * 36 / 100;
    int minpw = (int)(360 * s);
    if (pw < minpw)
        pw = minpw;
    if (pw > w - minpw)
        pw = w - minpw;
    if (pw > w / 2)
        pw = w / 2;
    if (pw < 200)
        pw = 200;
    SDL_Rect p = { w - pw, a->title_h, pw, h - a->title_h };
    return p;
}

static void col_layout(App *a, int w, int h, SDL_Rect *panel, SDL_Rect *hero, SDL_Rect *button)
{
    int pad = a->pad;
    int gap = (int)(14 * a->scale);
    SDL_Rect p = panel_rect(a, w, h);
    *panel = p;
    int cw = p.w - pad * 2;
    int name_h = (int)(46 * a->scale);
    int hh = cw * 3 / 4;
    int maxh = p.h / 3;
    if (hh > maxh)
        hh = maxh;
    hero->w = hh * 4 / 3;
    hero->x = p.x + pad + (cw - hero->w) / 2;
    hero->y = p.y + name_h + gap;
    hero->h = hh;
    int bh = (int)(48 * a->scale);
    button->x = p.x + pad;
    button->y = p.y + p.h - pad - bh;
    button->w = cw;
    button->h = bh;
}

static int mouse_over_panel(App *a, int w, int h)
{
    SDL_Rect panel, hero, button;
    col_layout(a, w, h, &panel, &hero, &button);
    return a->mouse_x >= panel.x && a->mouse_x < panel.x + panel.w && a->mouse_y >= panel.y &&
           a->mouse_y < panel.y + panel.h;
}

static void layout(App *a, SDL_Rect *grid)
{
    int w = BASE_W, h = BASE_H;
    SDL_GetRendererOutputSize(a->ren, &w, &h);
    SDL_Rect panel = panel_rect(a, w, h);
    grid->x = 0;
    grid->y = a->title_h;
    grid->w = panel.x;
    grid->h = h - a->title_h;
}

static int compute_cols(const App *a, const SDL_Rect *grid)
{
    int avail = grid->w - a->grid_pad * 2 + a->tile_gap;
    int cols = avail / (a->tile_w + a->tile_gap);
    if (cols < 1)
        cols = 1;
    return cols;
}

static SDL_Rect tile_rect(const App *a, const SDL_Rect *grid, int i, int cols)
{
    int col = i % cols;
    int row = i / cols;
    int used = cols * a->tile_w + (cols - 1) * a->tile_gap;
    int ox = (grid->w - used) / 2;
    if (ox < a->grid_pad)
        ox = a->grid_pad;
    SDL_Rect r;
    r.x = grid->x + ox + col * (a->tile_w + a->tile_gap);
    r.y = grid->y + a->grid_pad + row * (a->tile_h + a->tile_gap) - (int)a->scroll;
    r.w = a->tile_w;
    r.h = a->tile_h;
    return r;
}

static void clamp_scroll(App *a, const SDL_Rect *grid)
{
    int cols = compute_cols(a, grid);
    int rows = (a->lib->count + cols - 1) / cols;
    int content = a->grid_pad * 2 + rows * a->tile_h + (rows > 0 ? (rows - 1) * a->tile_gap : 0);
    int maxs = content - grid->h;
    if (maxs < 0)
        maxs = 0;
    if (a->scroll < 0)
        a->scroll = 0;
    if (a->scroll > maxs)
        a->scroll = maxs;
}

static int hit_test(App *a, const SDL_Rect *grid, int mx, int my)
{
    if (mx < grid->x || mx >= grid->x + grid->w || my < grid->y || my >= grid->y + grid->h)
        return -1;
    int cols = compute_cols(a, grid);
    for (int i = 0; i < a->lib->count; i++) {
        SDL_Rect r = tile_rect(a, grid, i, cols);
        if (r.y + r.h <= grid->y || r.y >= grid->y + grid->h)
            continue;
        if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h)
            return i;
    }
    return -1;
}

static void stop_video(App *a)
{
    if (a->video) {
        video_free(a->video);
        a->video = NULL;
    }
    a->video_playing = 0;
}

static void set_selected(App *a, int idx)
{
    if (idx == a->selected)
        return;
    a->selected = idx;
    a->hover_start = SDL_GetTicks();
    stop_video(a);
    if (idx >= 0 && idx < a->lib->count && a->lib->games[idx].video) {
        a->video = video_open(a->ren, a->lib->games[idx].video, 0);
        if (a->video)
            video_update(a->video, 0.02);
    }
}

static void scroll_into_view(App *a, const SDL_Rect *grid, int i)
{
    int cols = compute_cols(a, grid);
    SDL_Rect r = tile_rect(a, grid, i, cols);
    if (r.y < grid->y)
        a->scroll -= (grid->y - r.y);
    else if (r.y + r.h > grid->y + grid->h)
        a->scroll += (r.y + r.h - (grid->y + grid->h));
    clamp_scroll(a, grid);
}

static void move_selection(App *a, const SDL_Rect *grid, int dx, int dy)
{
    if (a->lib->count == 0)
        return;
    int cols = compute_cols(a, grid);
    int i = a->selected < 0 ? 0 : a->selected;
    if (dx) {
        i += dx;
        if (i < 0)
            i = 0;
        if (i >= a->lib->count)
            i = a->lib->count - 1;
    }
    if (dy) {
        i += dy * cols;
        if (i < 0)
            i = 0;
        if (i >= a->lib->count)
            i = a->lib->count - 1;
    }
    set_selected(a, i);
    scroll_into_view(a, grid, i);
}

static void launch(App *a)
{
    int i = a->selected;
    if (i < 0 || i >= a->lib->count)
        return;
    Game *g = &a->lib->games[i];
    const char *target = g->shortcut ? g->shortcut : g->exe;
    if (!target) {
        app_status(a, "No hay lanzador (.lnk ni .exe) en la carpeta");
        return;
    }
    if (launcher_open(target, g->dir) == 0) {
        int sel = a->selected;
        a->selected = -1;
        a->pinned = -1;
        stop_video(a);
        a->leave_time = SDL_GetTicks() - 1000000;
        char buf[300];
        SDL_snprintf(buf, sizeof(buf), "Abriendo %s ...", g->name);
        app_status(a, buf);
    } else {
        app_status(a, "No se pudo abrir el juego");
    }
}

static void text_ellipsized(SDL_Renderer *ren, TTF_Font *f, const char *s, SDL_Color c, SDL_Rect box)
{
    if (!f || !s)
        return;
    if (text_measure(f, s) <= box.w) {
        text_draw(ren, f, s, c, box.x, box.y);
        return;
    }
    char buf[512];
    size_t n = strlen(s);
    buf[0] = 0;
    for (size_t len = n; len > 0; len--) {
        if (len + 4 >= sizeof(buf))
            continue;
        memcpy(buf, s, len);
        buf[len] = 0;
        strcat(buf, "...");
        if (text_measure(f, buf) <= box.w)
            break;
    }
    text_draw(ren, f, buf, c, box.x, box.y);
}

static int ellipsize(TTF_Font *f, const char *s, int maxw, char *out, size_t outsz)
{
    if (!f || !s) {
        if (out && outsz)
            out[0] = 0;
        return 0;
    }
    int w = text_measure(f, s);
    if (w <= maxw) {
        SDL_snprintf(out, outsz, "%s", s);
        return w;
    }
    size_t n = strlen(s);
    out[0] = 0;
    for (size_t len = n; len > 0; len--) {
        if (len + 4 >= outsz)
            continue;
        memcpy(out, s, len);
        out[len] = 0;
        strcat(out, "...");
        if (text_measure(f, out) <= maxw)
            return text_measure(f, out);
    }
    SDL_snprintf(out, outsz, "...");
    return text_measure(f, out);
}

static void update(App *a)
{
    Uint32 now = SDL_GetTicks();
    a->frame_dt = (now - a->frame_last) / 1000.0;
    a->frame_last = now;

    if (a->video && !a->video_playing && SDL_TICKS_PASSED(now, a->hover_start + VIDEO_DELAY_MS)) {
        a->video_playing = 1;
        video_play(a->video);
    }

    if (a->mouse_active && a->pinned < 0 && a->selected >= 0) {
        int w = BASE_W, h = BASE_H;
        SDL_GetRendererOutputSize(a->ren, &w, &h);
        SDL_Rect grid;
        layout(a, &grid);
        int over_tile = hit_test(a, &grid, a->mouse_x, a->mouse_y) >= 0;
        int over_panel = mouse_over_panel(a, w, h);
        if (over_tile || over_panel)
            a->leave_time = now;
        else if (SDL_TICKS_PASSED(now, a->leave_time + HOVER_GRACE_MS))
            set_selected(a, -1);
    }
}

static void draw_button(App *a, SDL_Rect button, const char *label)
{
    Fonts *fn = a->fonts;
    int hov = a->mouse_x >= button.x && a->mouse_x < button.x + button.w &&
              a->mouse_y >= button.y && a->mouse_y < button.y + button.h;
    int br = a->radius + (int)(2 * a->scale);

    round_rect_c(a->ren, button, br, BTN_D);
    SDL_Rect inner = { button.x + 1, button.y + 1, button.w - 2, button.h - 2 };
    if (hov)
        vgrad_round(a->ren, inner, br > 1 ? br - 1 : 0, BTN_HOV_L, BTN_HOV_M);
    else
        vgrad_round(a->ren, inner, br > 1 ? br - 1 : 0, BTN_L, BTN_M);
    gloss_strip(a->ren, inner, br > 1 ? br - 1 : 0, inner.h / 2, hov ? 120 : 95);

    if (fn) {
        int tw = text_measure(fn->body, label);
        int tx = button.x + (button.w - tw) / 2;
        int ty = button.y + (button.h - TTF_FontHeight(fn->body)) / 2;
        text_draw(a->ren, fn->body, label, COL_WHITE, tx, ty);
    }
}

static void draw_placeholder(App *a, SDL_Rect box)
{
    SDL_Renderer *ren = a->ren;
    Fonts *fn = a->fonts;
    SDL_SetRenderDrawColor(ren, COL_PH_BG.r, COL_PH_BG.g, COL_PH_BG.b, 255);
    SDL_RenderFillRect(ren, &box);
    int cx = box.x + box.w / 2;
    int cy = box.y + box.h / 2;
    int r = box.w < box.h ? box.w : box.h;
    r = r * 9 / 20;
    fill_circle(ren, cx, cy, r, COL_TILE_PH);
    ring(ren, cx, cy, r, COL_PH_RING);
    if (fn) {
        int tw = text_measure(fn->title, "?");
        int th = TTF_FontHeight(fn->title);
        text_draw(ren, fn->title, "?", COL_PH_FG, cx - tw / 2, cy - th / 2 + 2);
        int lh = TTF_FontHeight(fn->small);
        SDL_Rect cap = { box.x, cy + r + (int)(8 * a->scale), box.w, lh };
        text_draw_centered(ren, fn->small, "Sin preview", COL_DIM, cap);
    }
}

static void row_line(App *a, TTF_Font *fn, const char *key, const char *val, int x, int y, int w,
                     int labelw, int lh)
{
    if (!fn)
        return;
    int th = TTF_FontHeight(fn);
    int ty = y + (lh - th) / 2;
    SDL_Rect kb = { x, ty, labelw - (int)(10 * a->scale), th };
    text_ellipsized(a->ren, fn, key, COL_KEY, kb);
    SDL_Rect vb = { x + labelw, ty, w - labelw, th };
    text_ellipsized(a->ren, fn, val, COL_TEXT, vb);
}

static void draw_column(App *a, int w, int h)
{
    SDL_Rect panel, hero, button;
    col_layout(a, w, h, &panel, &hero, &button);
    Fonts *fn = a->fonts;
    double s = a->scale;
    int pad = a->pad;
    int gap = (int)(14 * s);

    vgrad(a->ren, panel, COL_PANEL_TOP, COL_PANEL_BOT);
    SDL_SetRenderDrawColor(a->ren, COL_PANEL_EDGE.r, COL_PANEL_EDGE.g, COL_PANEL_EDGE.b, 255);
    SDL_Rect lb = { panel.x, panel.y, 1, panel.h };
    SDL_RenderFillRect(a->ren, &lb);

    int hdr_h = (int)(46 * s);
    SDL_Rect hdr = { panel.x, panel.y, panel.w, hdr_h };
    hgrad3(a->ren, hdr, COL_HDR_L, COL_HDR_M, COL_HDR_R);
    gloss_strip(a->ren, hdr, 0, hdr_h / 2, 70);
    SDL_Rect hline = { panel.x, panel.y + hdr_h - 1, panel.w, 1 };
    SDL_SetRenderDrawColor(a->ren, XP_FRAME.r, XP_FRAME.g, XP_FRAME.b, 255);
    SDL_RenderFillRect(a->ren, &hline);

    if (!fn)
        return;

    int x = panel.x + pad;
    int cw = panel.w - pad * 2;
    int lh_f = TTF_FontHeight(fn->small);

    if (a->selected < 0 || a->selected >= a->lib->count) {
        SDL_Rect mb = { x, panel.y + hdr_h + (panel.h - hdr_h) / 2 - lh_f / 2, cw, lh_f };
        text_draw_centered(a->ren, fn->small, "Pasa el cursor sobre un juego", COL_DIM, mb);
        return;
    }

    Game *g = &a->lib->games[a->selected];

    SDL_Rect hb = { x, panel.y, cw, hdr_h };
    text_draw_fitted(a->ren, fn->title, g->name, COL_WHITE, hb, fn->title_sz);

    SDL_SetRenderDrawColor(a->ren, COL_HERO_BG.r, COL_HERO_BG.g, COL_HERO_BG.b, 255);
    SDL_RenderFillRect(a->ren, &hero);
    if (a->video) {
        if (a->video_playing)
            video_update(a->video, a->frame_dt);
        video_draw(a->video, a->ren, &hero);
    } else {
        int iw = 0, ih = 0;
        SDL_Texture *ic = app_cover(a, a->selected, &iw, &ih);
        if (!ic) {
            ic = a->def_icon;
            iw = a->def_w;
            ih = a->def_h;
        }
        if (ic) {
            SDL_Rect f = fit_into(iw, ih, hero);
            SDL_RenderCopy(a->ren, ic, NULL, &f);
        } else {
            draw_placeholder(a, hero);
        }
    }
    SDL_SetRenderDrawColor(a->ren, COL_TILE_EDGE.r, COL_TILE_EDGE.g, COL_TILE_EDGE.b, 255);
    SDL_RenderDrawRect(a->ren, &hero);

    int y = hero.y + hero.h + gap;
    int lh = TTF_FontHeight(fn->small) + (int)(12 * s);
    int labelw = cw * 42 / 100;
    int limit = button.y - gap;

    static const char *def_keys[5] = { "Género", "Año", "Jugadores", "Desarrollador", "Distribuidora" };
    int rows = g->meta_count ? g->meta_count : 5;
    for (int k = 0; k < rows && y + lh <= limit; k++) {
        const char *key = g->meta_count ? g->meta_keys[k] : def_keys[k];
        const char *val = g->meta_count ? g->meta_vals[k] : (const char *)"?";
        if (strcmp(key, "Plataforma") == 0)
            continue;
        row_line(a, fn->small, key, val, x, y, cw, labelw, lh);
        SDL_SetRenderDrawColor(a->ren, COL_ROW_LINE.r, COL_ROW_LINE.g, COL_ROW_LINE.b, 255);
        SDL_RenderDrawLine(a->ren, x, y + lh, x + cw, y + lh);
        y += lh;
    }

    if (g->desc_text && y + (int)(10 * s) < limit) {
        y += gap;
        int sh = TTF_FontHeight(fn->small);
        text_draw(a->ren, fn->small, "DESCRIPCION", XP_BLUE2, x, y);
        y += sh + (int)(4 * s);
        SDL_Rect db = { x, y, cw, limit - y };
        text_draw_wrapped(a->ren, fn->small, g->desc_text, COL_DIM, db);
    }

    draw_button(a, button, "JUGAR");
}

static SDL_Rect theme_rect(App *a, int w)
{
    double s = a->scale;
    int sz = a->title_h - (int)(16 * s);
    if (sz < 22)
        sz = 22;
    int m = (int)(10 * s);
    SDL_Rect r = { w - m - sz, (a->title_h - sz) / 2, sz, sz };
    return r;
}

static void draw_theme_button(App *a, int w)
{
    SDL_Rect r = theme_rect(a, w);
    double s = a->scale;
    int rad = (int)(4 * s);
    int hov = a->mouse_x >= r.x && a->mouse_x < r.x + r.w && a->mouse_y >= r.y &&
              a->mouse_y < r.y + r.h;
    int next = (a->theme + 1) % THEME_COUNT;
    const Theme *t = &THEMES[next];

    round_rect_c(a->ren, r, rad, hov ? COL_HDR_M : COL_HDR_L);

    int cx = r.x + r.w / 2;
    int cy = r.y + r.h / 2;
    int R = r.w / 2 - (int)(6 * s);
    if (R < 4)
        R = 4;
    SDL_Color dots[4] = { t->title_l, t->blue, t->btn_m, t->hdr_m };
    draw_palette_icon(a->ren, cx, cy, R, (SDL_Color){ 236, 239, 245, 255 },
                      hov ? COL_HDR_M : COL_HDR_L, dots, 4);
}

static void draw(App *a)
{
    int w = BASE_W, h = BASE_H;
    SDL_GetRendererOutputSize(a->ren, &w, &h);
    SDL_Rect grid;
    layout(a, &grid);
    clamp_scroll(a, &grid);
    int cols = compute_cols(a, &grid);
    Fonts *fn = a->fonts;
    double s = a->scale;
    int rad = a->radius;

    SDL_SetRenderDrawColor(a->ren, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(a->ren);
    SDL_Rect full = { 0, 0, w, h };
    vgrad(a->ren, full, BG_TOP, BG_BOT);

    SDL_Rect tb = { 0, 0, w, a->title_h };
    hgrad3(a->ren, tb, TITLE_L, TITLE_M, TITLE_R);
    gloss_strip(a->ren, tb, 0, a->title_h / 2, 80);
    SDL_Rect tline = { 0, a->title_h - 1, w, 1 };
    SDL_SetRenderDrawColor(a->ren, XP_FRAME.r, XP_FRAME.g, XP_FRAME.b, 255);
    SDL_RenderFillRect(a->ren, &tline);

    if (fn) {
        int th = TTF_FontHeight(fn->title);
        text_draw(a->ren, fn->title, "CIBER AHERO", COL_WHITE, (int)(16 * s), (a->title_h - th) / 2);
    }
    draw_theme_button(a, w);

    if (fn && a->status[0] && !SDL_TICKS_PASSED(SDL_GetTicks(), a->status_until)) {
        char buf[256];
        int maxw = (int)(w * 0.55);
        int th = TTF_FontHeight(fn->body);
        int stw = ellipsize(fn->body, a->status, maxw, buf, sizeof(buf));
        int px = (w - stw) / 2;
        int py = (a->title_h - th) / 2;
        int padx = (int)(12 * s);
        int pady = (int)(4 * s);
        SDL_Rect badge = { px - padx, py - pady, stw + padx * 2, th + pady * 2 };
        round_rect_c(a->ren, badge, (int)(6 * s), (SDL_Color){ 0, 0, 0, 150 });
        text_draw(a->ren, fn->body, buf, COL_WHITE, px, py);
    }

    int icon_top = (int)(12 * s);

    SDL_RenderSetClipRect(a->ren, &grid);

    for (int i = 0; i < a->lib->count; i++) {
        SDL_Rect r = tile_rect(a, &grid, i, cols);
        if (r.y + r.h <= grid.y || r.y >= grid.y + grid.h)
            continue;
        int sel = (i == a->selected);

        SDL_Color border = sel ? XP_BLUE : COL_TILE_EDGE;
        round_rect_c(a->ren, r, rad, border);
        int inset = (int)(2 * s);
        SDL_Rect body = { r.x + inset, r.y + inset, r.w - inset * 2, r.h - inset * 2 };
        round_rect_c(a->ren, body, rad - inset, sel ? COL_TILE_SEL : COL_TILE);

        SDL_Rect ib = { r.x + (a->tile_w - a->icon_box) / 2, r.y + icon_top, a->icon_box, a->icon_box };
        SDL_Texture *ic = app_icon(a, i);
        if (ic) {
            SDL_Rect f = fit_into(a->icons_w[i], a->icons_h[i], ib);
            SDL_RenderCopy(a->ren, ic, NULL, &f);
        } else {
            round_rect_c(a->ren, ib, (int)(4 * s), COL_TILE_PH);
            text_draw_centered(a->ren, fn ? fn->body : NULL, "?", COL_TILE_PH_FG, ib);
        }

        if (fn) {
            int lb_top = ib.y + a->icon_box + (int)(8 * s);
            SDL_Rect lb = { r.x + (int)(6 * s), lb_top, a->tile_w - (int)(12 * s),
                            (r.y + r.h) - lb_top - (int)(6 * s) };
            if (lb.h < (int)(22 * s))
                lb.h = (int)(22 * s);
            text_draw_fitted_wrapped(a->ren, fn->small, a->lib->games[i].name,
                                     sel ? COL_SEL_TEXT : COL_TEXT, lb, fn->small_sz);
        }
    }

    SDL_RenderSetClipRect(a->ren, NULL);

    if (a->lib->count == 0) {
        text_draw_centered(a->ren, fn ? fn->body : NULL, "No se encontraron juegos", COL_TEXT, grid);
        int y = grid.y + grid.h / 2 + (int)(30 * s);
        SDL_Rect line = { grid.x + (int)(20 * s), y, grid.w - (int)(40 * s), (int)(60 * s) };
        char msg[512];
        SDL_snprintf(msg, sizeof(msg), "Biblioteca: %s", a->lib->root);
        text_draw_centered(a->ren, fn ? fn->small : NULL, msg, COL_DIM, line);
    }

    draw_column(a, w, h);
}

static void toggle_fullscreen(App *a)
{
    Uint32 flags = SDL_GetWindowFlags(a->win);
    if (flags & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(a->win, 0);
    else
        SDL_SetWindowFullscreen(a->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

int app_run(const char *library_root, const struct LaptopConfig *cfg)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cyber AHERO", SDL_GetError(), NULL);
        return 1;
    }
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_Window *win = SDL_CreateWindow("Cyber AHERO", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, BASE_W, BASE_H,
                                       SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
    if (!win) {
        win = SDL_CreateWindow("Cyber AHERO", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               BASE_W, BASE_H, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    }
    if (!win) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cyber AHERO", SDL_GetError(), NULL);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cyber AHERO", SDL_GetError(), NULL);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    App a;
    memset(&a, 0, sizeof(a));
    a.win = win;
    a.ren = ren;
    a.running = 1;
    a.selected = -1;
    a.pinned = -1;
    a.frame_last = SDL_GetTicks();

    app_metrics(&a);

    a.lib = library_scan(fs_win(), library_root, cfg);
    int cap = a.lib->count > 0 ? a.lib->count : 1;
    a.icons = calloc((size_t)cap, sizeof(SDL_Texture *));
    a.icon_tried = calloc((size_t)cap, 1);
    a.icons_w = calloc((size_t)cap, sizeof(int));
    a.icons_h = calloc((size_t)cap, sizeof(int));
    a.covers = calloc((size_t)cap, sizeof(SDL_Texture *));
    a.cover_tried = calloc((size_t)cap, 1);
    a.covers_w = calloc((size_t)cap, sizeof(int));
    a.covers_h = calloc((size_t)cap, sizeof(int));
    a.theme = 0;
    theme_apply(a.theme);
    a.fonts = fonts_load(a.scale);
    a.def_icon = create_default_icon(&a);

    while (a.running) {
        SDL_Rect grid;
        layout(&a, &grid);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                a.running = 0;
                break;
            case SDL_MOUSEMOTION:
                a.mouse_x = e.motion.x;
                a.mouse_y = e.motion.y;
                a.mouse_active = 1;
                {
                    int idx = hit_test(&a, &grid, e.motion.x, e.motion.y);
                    if (idx >= 0 && a.pinned < 0)
                        set_selected(&a, idx);
                    int w = BASE_W, h = BASE_H;
                    SDL_GetRendererOutputSize(ren, &w, &h);
                    if (idx >= 0 || mouse_over_panel(&a, w, h))
                        a.leave_time = SDL_GetTicks();
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                a.mouse_active = 1;
                if (e.button.button == SDL_BUTTON_LEFT) {
                    int w = BASE_W, h = BASE_H;
                    SDL_GetRendererOutputSize(ren, &w, &h);
                    SDL_Rect panel, hero, button;
                    col_layout(&a, w, h, &panel, &hero, &button);
                    SDL_Rect tr = theme_rect(&a, w);
                    if (e.button.x >= tr.x && e.button.x < tr.x + tr.w &&
                        e.button.y >= tr.y && e.button.y < tr.y + tr.h) {
                        a.theme = (a.theme + 1) % THEME_COUNT;
                        theme_apply(a.theme);
                        if (a.def_icon)
                            SDL_DestroyTexture(a.def_icon);
                        a.def_icon = create_default_icon(&a);
                        a.selected = -1;
                        a.pinned = -1;
                        app_status(&a, THEMES[a.theme].name);
                    } else if (e.button.x >= button.x && e.button.x < button.x + button.w &&
                        e.button.y >= button.y && e.button.y < button.y + button.h) {
                        launch(&a);
                        a.selected = -1; a.pinned = -1;
                    } else {
                        int idx = hit_test(&a, &grid, e.button.x, e.button.y);
                        if (idx >= 0) {
                            a.pinned = idx;
                            set_selected(&a, idx);
                            if (e.button.clicks == 2) {
                                launch(&a);
                                a.selected = -1; a.pinned = -1;
                            }
                        } else {
                            a.pinned = -1;
                            set_selected(&a, -1);
                        }
                    }
                }
                break;
            case SDL_MOUSEWHEEL:
                a.scroll -= e.wheel.y * (int)(48 * a.scale);
                clamp_scroll(&a, &grid);
                break;
            case SDL_KEYDOWN:
                a.mouse_active = 0;
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (a.pinned >= 0) {
                        a.pinned = -1;
                        set_selected(&a, -1);
                    } else {
                        a.running = 0;
                    }
                    break;
                case SDLK_F11:
                    toggle_fullscreen(&a);
                    break;
                case SDLK_RETURN:
                    if (e.key.keysym.mod & KMOD_ALT)
                        toggle_fullscreen(&a);
                    else {
                        launch(&a);
                        a.selected = -1; a.pinned = -1;
                    }
                    break;
                case SDLK_SPACE:
                    launch(&a);
                    a.selected = -1; a.pinned = -1;
                    break;
                case SDLK_LEFT:
                    move_selection(&a, &grid, -1, 0);
                    a.pinned = a.selected;
                    break;
                case SDLK_RIGHT:
                    move_selection(&a, &grid, 1, 0);
                    a.pinned = a.selected;
                    break;
                case SDLK_UP:
                    move_selection(&a, &grid, 0, -1);
                    a.pinned = a.selected;
                    break;
                case SDLK_DOWN:
                    move_selection(&a, &grid, 0, 1);
                    a.pinned = a.selected;
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        update(&a);
        draw(&a);
        SDL_RenderPresent(ren);

        Uint32 elapsed = SDL_GetTicks() - a.frame_last;
        if (elapsed < 16)
            SDL_Delay(16 - elapsed);
    }

    stop_video(&a);
    for (int i = 0; i < a.lib->count; i++) {
        if (a.icons[i])
            SDL_DestroyTexture(a.icons[i]);
        if (a.covers[i])
            SDL_DestroyTexture(a.covers[i]);
    }
    free(a.icons);
    free(a.icon_tried);
    free(a.icons_w);
    free(a.icons_h);
    free(a.covers);
    free(a.cover_tried);
    free(a.covers_w);
    free(a.covers_h);
    fonts_free(a.fonts);
    if (a.def_icon)
        SDL_DestroyTexture(a.def_icon);
    library_free(a.lib);
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
