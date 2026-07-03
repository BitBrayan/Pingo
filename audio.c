#include "audio.h"
#include <SDL2/SDL.h>
#include <stdio.h>

#if defined(__has_include)
#  if __has_include(<SDL2/SDL_mixer.h>)
#    include <SDL2/SDL_mixer.h>
#    define HAVE_SDL_MIXER 1
#  endif
#endif

#ifdef HAVE_SDL_MIXER
static Mix_Chunk *g_sfx[SFX_COUNT] = { NULL };
static Mix_Music *g_music = NULL;
static int g_music_volume = MIX_MAX_VOLUME / 3;  /* padrão ~33% */
static int g_sfx_volume   = MIX_MAX_VOLUME / 3;  /* padrão ~33% */

static int clamp_percent(int percent) {
    if (percent < 0) return 0;
    if (percent > 100) return 100;
    return percent;
}

int audio_init(void) {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Mix_OpenAudio error: %s\n", Mix_GetError());
        return -1;
    }

    /* Tenta carregar os efeitos sonoros comuns de ./sounds/ */
    g_sfx[SFX_HIT] = Mix_LoadWAV("sounds/hit.wav");
    if (!g_sfx[SFX_HIT]) fprintf(stderr, "Aviso: hit.wav não encontrado: %s\n", Mix_GetError());

    g_sfx[SFX_SCORE] = Mix_LoadWAV("sounds/score.wav");
    if (!g_sfx[SFX_SCORE]) fprintf(stderr, "Aviso: score.wav não encontrado: %s\n", Mix_GetError());

    g_sfx[SFX_MENU] = Mix_LoadWAV("sounds/menu.wav");
    if (!g_sfx[SFX_MENU]) fprintf(stderr, "Aviso: menu.wav não encontrado: %s\n", Mix_GetError());

    audio_set_music_volume((g_music_volume * 100) / MIX_MAX_VOLUME);
    audio_set_sfx_volume((g_sfx_volume * 100) / MIX_MAX_VOLUME);

    return 0;
}

void audio_destroy(void) {
    for (int i = 0; i < SFX_COUNT; i++) {
        if (g_sfx[i]) { Mix_FreeChunk(g_sfx[i]); g_sfx[i] = NULL; }
    }
    if (g_music) { Mix_HaltMusic(); Mix_FreeMusic(g_music); g_music = NULL; }
    Mix_CloseAudio();
}

void audio_play_sfx(int id) {
    if (id < 0 || id >= SFX_COUNT) return;
    if (!g_sfx[id]) return;
    Mix_VolumeChunk(g_sfx[id], g_sfx_volume);
    Mix_PlayChannel(-1, g_sfx[id], 0);
}

int audio_set_music_volume(int percent) {
    percent = clamp_percent(percent);
    g_music_volume = percent * MIX_MAX_VOLUME / 100;
    Mix_VolumeMusic(g_music_volume);
    return percent;
}

int audio_set_sfx_volume(int percent) {
    percent = clamp_percent(percent);
    g_sfx_volume = percent * MIX_MAX_VOLUME / 100;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (g_sfx[i]) Mix_VolumeChunk(g_sfx[i], g_sfx_volume);
    }
    return percent;
}

int audio_play_music(const char *path) {
    if (!path) return -1;
    if (g_music) { Mix_HaltMusic(); Mix_FreeMusic(g_music); g_music = NULL; }
    g_music = Mix_LoadMUS(path);
    if (!g_music) {
        fprintf(stderr, "Aviso: música não encontrada %s: %s\n", path, Mix_GetError());
        return -1;
    }
    if (Mix_PlayMusic(g_music, -1) != 0) {
        fprintf(stderr, "Erro ao tocar música: %s\n", Mix_GetError());
        return -1;
    }
    return 0;
}

void audio_stop_music(void) {
    if (g_music) Mix_HaltMusic();
}

#else
/* Funções de substituição quando SDL2_mixer não estiver disponível */
int audio_init(void) {
    fprintf(stderr, "Audio: SDL2_mixer não disponível — áudio desativado.\n");
    return -1;
}
void audio_destroy(void) { }
void audio_play_sfx(int id) { (void)id; }
int audio_set_music_volume(int percent) { (void)percent; return -1; }
int audio_set_sfx_volume(int percent) { (void)percent; return -1; }
int audio_play_music(const char *path) { (void)path; return -1; }
void audio_stop_music(void) { }
#endif
