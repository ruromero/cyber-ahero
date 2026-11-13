#include "video.h"

#include <stdio.h>
#include "pl_mpeg.h"

#include <stdlib.h>
#include <string.h>

struct Video {
    plm_t *plm;
    SDL_Texture *tex;
    SDL_AudioDeviceID audio;
    unsigned char *rgba;
    int width;
    int height;
    int ends;
};

static void on_video(plm_t *self, plm_frame_t *frame, void *user)
{
    Video *v = (Video *)user;
    (void)self;
    plm_frame_to_rgba(frame, v->rgba, v->width * 4);
    SDL_UpdateTexture(v->tex, NULL, v->rgba, v->width * 4);
}

static void on_audio(plm_t *self, plm_samples_t *samples, void *user)
{
    Video *v = (Video *)user;
    (void)self;
    if (!v->audio)
        return;

    int count = (int)samples->count;
    static Sint16 buf[PLM_AUDIO_SAMPLES_PER_FRAME * 2];
    for (int i = 0; i < count * 2; i++) {
        float f = samples->interleaved[i];
        if (f > 1.0f)
            f = 1.0f;
        if (f < -1.0f)
            f = -1.0f;
        buf[i] = (Sint16)(f * 32767.0f);
    }
    SDL_QueueAudio(v->audio, buf, (Uint32)(count * 2 * sizeof(Sint16)));
}

Video *video_open(SDL_Renderer *renderer, const char *path, int enable_audio)
{
    Video *v = calloc(1, sizeof(Video));
    if (!v)
        return NULL;

    v->plm = plm_create_with_filename(path);
    if (!v->plm) {
        free(v);
        return NULL;
    }

    v->width = plm_get_width(v->plm);
    v->height = plm_get_height(v->plm);
    if (v->width <= 0 || v->height <= 0) {
        plm_destroy(v->plm);
        free(v);
        return NULL;
    }

    v->tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                               v->width, v->height);
    if (!v->tex) {
        plm_destroy(v->plm);
        free(v);
        return NULL;
    }
    SDL_SetTextureBlendMode(v->tex, SDL_BLENDMODE_NONE);

    v->rgba = malloc((size_t)v->width * (size_t)v->height * 4);
    if (!v->rgba) {
        SDL_DestroyTexture(v->tex);
        plm_destroy(v->plm);
        free(v);
        return NULL;
    }

    if (enable_audio) {
        plm_set_audio_enabled(v->plm, 1);
        int rate = plm_get_samplerate(v->plm);
        if (rate > 0) {
            SDL_AudioSpec want, have;
            SDL_zero(want);
            want.freq = rate;
            want.format = AUDIO_S16SYS;
            want.channels = 2;
            want.samples = 2048;
            v->audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
            if (v->audio) {
                plm_set_audio_decode_callback(v->plm, on_audio, v);
                plm_set_audio_lead_time(v->plm, 0.08);
                SDL_PauseAudioDevice(v->audio, 1);
            }
        }
    }

    plm_set_video_decode_callback(v->plm, on_video, v);
    return v;
}

void video_free(Video *v)
{
    if (!v)
        return;
    if (v->audio)
        SDL_CloseAudioDevice(v->audio);
    if (v->plm)
        plm_destroy(v->plm);
    if (v->tex)
        SDL_DestroyTexture(v->tex);
    free(v->rgba);
    free(v);
}

void video_update(Video *v, double dt)
{
    if (!v || !v->plm)
        return;
    if (dt < 0)
        dt = 0;
    if (dt > 0.1)
        dt = 0.1;

    plm_decode(v->plm, dt);
    if (plm_has_ended(v->plm)) {
        plm_rewind(v->plm);
        v->ends++;
        if (v->audio)
            SDL_ClearQueuedAudio(v->audio);
    }
}

void video_draw(Video *v, SDL_Renderer *renderer, const SDL_Rect *dst)
{
    if (!v || !v->tex)
        return;

    double sx = (double)dst->w / (double)v->width;
    double sy = (double)dst->h / (double)v->height;
    double s = sx < sy ? sx : sy;
    int w = (int)(v->width * s);
    int h = (int)(v->height * s);
    SDL_Rect r;
    r.w = w;
    r.h = h;
    r.x = dst->x + (dst->w - w) / 2;
    r.y = dst->y + (dst->h - h) / 2;
    SDL_RenderCopy(renderer, v->tex, NULL, &r);
}

void video_play(Video *v)
{
    if (v && v->audio)
        SDL_PauseAudioDevice(v->audio, 0);
}

int video_valid(const Video *v)
{
    return v && v->tex;
}

int video_width(const Video *v)
{
    return v ? v->width : 0;
}

int video_height(const Video *v)
{
    return v ? v->height : 0;
}

int video_end_count(const Video *v)
{
    return v ? v->ends : 0;
}
