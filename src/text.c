#include "text.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

Fonts *fonts_load(double scale)
{
    if (TTF_Init() != 0)
        return NULL;

    char *path = font_path_from_system();
    if (!path)
        return NULL;
    char *title_path = font_path_title();
    if (!title_path)
        title_path = xstrdup(path);

    Fonts *f = calloc(1, sizeof(Fonts));
    if (!f) {
        free(path);
        free(title_path);
        return NULL;
    }

    int title_sz = (int)(30 * scale + 0.5);
    int body_sz = (int)(19 * scale + 0.5);
    int small_sz = (int)(15 * scale + 0.5);
    if (title_sz < 10)
        title_sz = 10;
    if (body_sz < 8)
        body_sz = 8;
    if (small_sz < 7)
        small_sz = 7;

    f->title_sz = title_sz;
    f->body_sz = body_sz;
    f->small_sz = small_sz;

    f->title = TTF_OpenFont(title_path, title_sz);
    f->body = TTF_OpenFont(path, body_sz);
    f->small = TTF_OpenFont(path, small_sz);
    if (!f->title)
        f->title = TTF_OpenFont(path, title_sz);
    if (!f->title || !f->body || !f->small) {
        fonts_free(f);
        free(path);
        free(title_path);
        return NULL;
    }
    free(path);
    free(title_path);
    return f;
}

void fonts_free(Fonts *f)
{
    if (!f)
        return;
    if (f->title)
        TTF_CloseFont(f->title);
    if (f->body)
        TTF_CloseFont(f->body);
    if (f->small)
        TTF_CloseFont(f->small);
    free(f);
}

void text_draw(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y)
{
    if (!font || !text || !*text)
        return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf)
        return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    if (!tex)
        return;
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

int text_measure(TTF_Font *font, const char *text)
{
    if (!font || !text || !*text)
        return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text, &w, &h);
    return w;
}

void text_draw_fitted(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                      SDL_Rect box, int base_size)
{
    if (!font || !text || !*text)
        return;
    int min_size = base_size * 55 / 100;
    if (min_size < 7)
        min_size = 7;

    int w = 0, h = 0;
    if (TTF_SizeUTF8(font, text, &w, &h) != 0)
        return;

    if (w > box.w && base_size > min_size) {
        int sz = base_size;
        for (sz = base_size - 1; sz > min_size; sz--) {
            TTF_SetFontSize(font, sz);
            if (TTF_SizeUTF8(font, text, &w, &h) != 0)
                break;
            if (w <= box.w)
                break;
        }
        if (sz <= min_size) {
            TTF_SetFontSize(font, min_size);
            TTF_SizeUTF8(font, text, &w, &h);
        }
        text_draw(ren, font, text, color, box.x, box.y + (box.h - h) / 2);
        TTF_SetFontSize(font, base_size);
    } else {
        text_draw(ren, font, text, color, box.x, box.y + (box.h - h) / 2);
    }
}

static int wrap_text(TTF_Font *font, const char *text, int maxw, char lines[][512], int store_max)
{
    int n = 0;
    char cur[512];
    cur[0] = 0;
    size_t cl = 0;
    const char *p = text;
    while (1) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        const char *ws = p;
        while (*p && *p != ' ')
            p++;
        size_t wl = (size_t)(p - ws);

        char cand[512];
        size_t c = 0;
        if (cl) {
            memcpy(cand, cur, cl);
            c = cl;
            cand[c++] = ' ';
        }
        size_t cpy = wl;
        if (c + cpy > 510)
            cpy = 510 - c;
        memcpy(cand + c, ws, cpy);
        c += cpy;
        cand[c] = 0;

        if (cl == 0 || text_measure(font, cand) <= maxw) {
            memcpy(cur, cand, c + 1);
            cl = c;
        } else {
            if (n < store_max)
                memcpy(lines[n], cur, cl + 1);
            n++;
            size_t c2 = wl > 511 ? 511 : wl;
            memcpy(cur, ws, c2);
            cur[c2] = 0;
            cl = c2;
        }
    }
    if (cl) {
        if (n < store_max)
            memcpy(lines[n], cur, cl + 1);
        n++;
    }
    return n;
}

void text_draw_fitted_wrapped(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                              SDL_Rect box, int base_size)
{
    if (!font || !text || !*text)
        return;

    int min_size = base_size * 55 / 100;
    if (min_size < 7)
        min_size = 7;

    char lines[8][512];

    for (int sz = base_size; sz >= min_size; sz--) {
        if (sz != base_size)
            TTF_SetFontSize(font, sz);

        int lskip = TTF_FontLineSkip(font);
        if (lskip <= 0)
            lskip = TTF_FontHeight(font);
        int maxl = lskip > 0 ? box.h / lskip : 1;
        if (maxl < 1)
            maxl = 1;
        if (maxl > 8)
            maxl = 8;

        int n = wrap_text(font, text, box.w, lines, 8);
        int wide = 0;
        for (int i = 0; i < n && i < 8; i++)
            if (text_measure(font, lines[i]) > box.w)
                wide = 1;

        int fit = (n <= maxl) && !wide;
        if (fit || sz == min_size) {
            if (n > maxl)
                n = maxl;
            int y = box.y;
            for (int i = 0; i < n; i++) {
                int lw = text_measure(font, lines[i]);
                text_draw(ren, font, lines[i], color, box.x + (box.w - lw) / 2, y + i * lskip);
            }
            TTF_SetFontSize(font, base_size);
            return;
        }
    }
    TTF_SetFontSize(font, base_size);
}

void text_draw_centered(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                        SDL_Rect box)
{
    if (!font || !text || !*text)
        return;
    int w = 0, h = 0;
    if (TTF_SizeUTF8(font, text, &w, &h) != 0)
        return;
    text_draw(ren, font, text, color, box.x + (box.w - w) / 2, box.y + (box.h - h) / 2);
}

void text_draw_wrapped(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                       SDL_Rect box)
{
    if (!font || !text)
        return;

    int line_h = TTF_FontLineSkip(font);
    int y = box.y;
    const char *p = text;

    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t seg_len = nl ? (size_t)(nl - p) : strlen(p);

        char seg[4096];
        size_t n = seg_len < sizeof(seg) - 1 ? seg_len : sizeof(seg) - 1;
        memcpy(seg, p, n);
        seg[n] = 0;

        if (n == 0) {
            y += line_h;
        } else {
            size_t i = 0;
            while (i < n) {
                size_t line_start = i;
                size_t fit_end = (size_t)-1;
                size_t j = i;
                char tmp[4096];

                while (j < n) {
                    size_t k = j;
                    while (k < n && seg[k] != ' ')
                        k++;
                    size_t cand = k - line_start;
                    if (cand >= sizeof(tmp))
                        cand = sizeof(tmp) - 1;
                    memcpy(tmp, seg + line_start, cand);
                    tmp[cand] = 0;
                    if (text_measure(font, tmp) <= box.w) {
                        fit_end = k;
                        if (k >= n)
                            break;
                        j = k + 1;
                    } else {
                        break;
                    }
                }

                size_t draw_end;
                if (fit_end == (size_t)-1) {
                    size_t k = 1;
                    while (line_start + k <= n) {
                        size_t c = k < sizeof(tmp) - 1 ? k : sizeof(tmp) - 1;
                        memcpy(tmp, seg + line_start, c);
                        tmp[c] = 0;
                        if (text_measure(font, tmp) > box.w)
                            break;
                        k++;
                    }
                    if (k > 1)
                        k--;
                    draw_end = line_start + k;
                } else {
                    draw_end = fit_end;
                }

                size_t dl = draw_end - line_start;
                while (dl > 0 && seg[line_start + dl - 1] == ' ')
                    dl--;
                size_t c = dl < sizeof(tmp) - 1 ? dl : sizeof(tmp) - 1;
                memcpy(tmp, seg + line_start, c);
                tmp[c] = 0;

                if (y + line_h > box.y + box.h)
                    return;
                if (c > 0)
                    text_draw(ren, font, tmp, color, box.x, y);
                y += line_h;

                i = draw_end;
                while (i < n && seg[i] == ' ')
                    i++;
            }
        }

        if (!nl)
            break;
        p = nl + 1;
    }
}
