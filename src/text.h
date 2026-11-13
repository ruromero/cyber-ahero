#ifndef TEXT_H
#define TEXT_H

#include <SDL.h>
#include <SDL_ttf.h>

typedef struct Fonts {
    TTF_Font *title;
    TTF_Font *body;
    TTF_Font *small;
    int title_sz;
    int body_sz;
    int small_sz;
} Fonts;

Fonts *fonts_load(double scale);
void fonts_free(Fonts *f);

void text_draw(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y);
void text_draw_centered(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                        SDL_Rect box);
void text_draw_fitted(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                      SDL_Rect box, int base_size);
void text_draw_fitted_wrapped(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                              SDL_Rect box, int base_size);
void text_draw_wrapped(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color,
                       SDL_Rect box);
int text_measure(TTF_Font *font, const char *text);

#endif
