#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "game.h"
#include "scoreboard.h"

int input_handle(SDL_Event *event, Game *g, Scoreboard *sb);

void input_update_paddles(Game *g);

#endif
