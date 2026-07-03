#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "game.h"
#include "scoreboard.h"

/* Processa eventos SDL; retorna 0 para continuar, 1 para sair */
int input_handle(SDL_Event *event, Game *g, Scoreboard *sb, SDL_Window *window);

void input_update_paddles(Game *g);

#endif
