/* audio.h - wrapper simples de efeitos sonoros e música usando SDL2_mixer */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

int audio_init(void);
void audio_destroy(void);

/* Controle de volume (0-100) sem editar os arquivos de áudio */
int audio_set_music_volume(int percent);
int audio_set_sfx_volume(int percent);

/* Toca um efeito sonoro pequeno pelo id (0..N-1) */
void audio_play_sfx(int id);

/* Toca a música de fundo (caminho), retorna 0 em caso de sucesso */
int audio_play_music(const char *path);
void audio_stop_music(void);

/* IDs de efeitos sonoros predefinidos */
enum {
    SFX_HIT = 0,
    SFX_SCORE,
    SFX_MENU,
    SFX_COUNT
};

#endif
