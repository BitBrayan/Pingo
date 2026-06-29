#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"
#include "scoreboard.h"

typedef struct {
    SDL_Renderer *renderer;
    TTF_Font     *font_lg;   /* 48px — placar */
    TTF_Font     *font_md;   /* 28px — mensagens */
    TTF_Font     *font_sm;   /* 18px — scoreboard */
} Renderer;

/* Inicializa renderer e carrega fontes */
int  render_init(Renderer *r, SDL_Renderer *sdl_renderer);
void render_destroy(Renderer *r);

/* Frame completo */
void render_frame(Renderer *r, const Game *g, const Scoreboard *sb);

#endif
