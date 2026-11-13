#define SDL_MAIN_HANDLED
#include "../src/video.h"

#include <SDL.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "uso: test_video <archivo.mpg>\n");
        return 2;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("t", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       320, 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    Video *v = video_open(ren, argv[1], 1);
    if (!video_valid(v)) {
        fprintf(stderr, "no se pudo abrir el video\n");
        return 1;
    }
    printf("video %dx%d\n", video_width(v), video_height(v));

    int frames = 0;
    for (int i = 0; i < 300; i++) {
        video_update(v, 1.0 / 60.0);
        video_draw(v, ren, &(SDL_Rect){ 0, 0, 320, 240 });
        SDL_RenderPresent(ren);
        frames++;
    }

    printf("decodificados %d ticks OK\n", frames);
    video_free(v);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
