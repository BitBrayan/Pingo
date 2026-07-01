#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"
#include "scoreboard.h"

typedef struct {
    SDL_Renderer *renderer;
    TTF_Font     *font_lg;   
    TTF_Font     *font_md;   
    TTF_Font     *font_sm; 
} Renderer;

int  render_init(Renderer *r, SDL_Renderer *sdl_renderer);
void render_destroy(Renderer *r);

void render_frame(Renderer *r, const Game *g, const Scoreboard *sb);

#endif
