#ifndef VIDEO_H
#define VIDEO_H

#include <SDL.h>

typedef struct Video Video;

Video *video_open(SDL_Renderer *renderer, const char *path, int enable_audio);
void video_free(Video *v);
void video_update(Video *v, double dt);
void video_draw(Video *v, SDL_Renderer *renderer, const SDL_Rect *dst);
void video_play(Video *v);
int video_valid(const Video *v);
int video_width(const Video *v);
int video_height(const Video *v);

#endif
