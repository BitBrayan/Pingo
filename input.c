#include "input.h"
#include "scoreboard.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static int key_w = 0, key_s = 0;      
static int key_up = 0, key_down = 0;  

static void save_result(Game *g, Scoreboard *sb) {
    ScoreEntry entry;
    memset(&entry, 0, sizeof(entry));

    snprintf(entry.p1_name, sizeof(entry.p1_name), "%s", g->p1_name);
    snprintf(entry.p2_name, sizeof(entry.p2_name), "%s", g->p2_name);
    entry.score_p1 = g->p1.score;
    entry.score_p2 = g->p2.score;
    entry.mode = g->mode;

    const char *winner = (g->p1.score > g->p2.score) ? g->p1_name : g->p2_name;
    snprintf(entry.winner, sizeof(entry.winner), "%s", winner);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(entry.date, sizeof(entry.date), "%Y-%m-%d %H:%M", tm_info);

    scoreboard_insert(sb, &entry);
    scoreboard_save(sb, SCOREBOARD_FILE);
}

int input_handle(SDL_Event *event, Game *g, Scoreboard *sb, SDL_Window *window) {
    if (event->type == SDL_QUIT) return 1;

    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;

        if (key == SDLK_f && (event->key.keysym.mod & KMOD_CTRL)) {
            Uint32 flags = SDL_GetWindowFlags(window);
            if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                SDL_SetWindowFullscreen(window, 0);
            } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
        }

        /* Teclas de movimento — armazena estado */
        if (key == SDLK_w)      key_w    = 1;
        if (key == SDLK_s)      key_s    = 1;
        if (key == SDLK_UP)     key_up   = 1;
        if (key == SDLK_DOWN)   key_down = 1;

        switch (g->state) {

        case STATE_MENU:
            if (key == SDLK_UP || key == SDLK_w) {
                g->menu_sel = (g->menu_sel + 3) % 4;
            } else if (key == SDLK_DOWN || key == SDLK_s) {
                g->menu_sel = (g->menu_sel + 1) % 4;
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                switch (g->menu_sel) {
                case 0: 
                    g->mode = MODE_1P;
                    strcpy(g->p2_name, "CPU");
                    g->name_input_field = 0;
                    memset(g->input_buf, 0, sizeof(g->input_buf));
                    g->input_len = 0;
                    SDL_StartTextInput();
                    g->state = STATE_NAME_INPUT;
                    break;
                case 1: 
                    g->mode = MODE_2P;
                    g->name_input_field = 0;
                    memset(g->input_buf, 0, sizeof(g->input_buf));
                    g->input_len = 0;
                    SDL_StartTextInput();
                    g->state = STATE_NAME_INPUT;
                    break;
                case 2: 
                    g->state = STATE_SCOREBOARD;
                    break;
                case 3: 
                    return 1;
                }
            }
            break;

        case STATE_NAME_INPUT:
            if (key == SDLK_BACKSPACE && g->input_len > 0) {
                g->input_buf[--g->input_len] = '\0';
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                if (g->input_len == 0) {
                    if (g->name_input_field == 0) strcpy(g->input_buf, "Jogador 1");
                    else                           strcpy(g->input_buf, "Jogador 2");
                }
                if (g->name_input_field == 0) {
                    snprintf(g->p1_name, sizeof(g->p1_name), "%s", g->input_buf);
                    if (g->mode == MODE_2P) {
                        g->name_input_field = 1;
                        memset(g->input_buf, 0, sizeof(g->input_buf));
                        g->input_len = 0;
                    } else {
                        SDL_StopTextInput();
                        game_start_match(g);
                    }
                } else {
                    snprintf(g->p2_name, sizeof(g->p2_name), "%s", g->input_buf);
                    SDL_StopTextInput();
                    game_start_match(g);
                }
            } else if (key == SDLK_ESCAPE) {
                SDL_StopTextInput();
                g->state = STATE_MENU;
            }
            break;

        case STATE_PLAYING:
            if (key == SDLK_p) {
                g->state = STATE_PAUSED;
            } else if (key == SDLK_ESCAPE) {
                g->state = STATE_MENU;
            }
            break;
        case STATE_PAUSED:
            if (key == SDLK_p || key == SDLK_ESCAPE) {
                g->state = STATE_PLAYING;
            }
            break;

        case STATE_GAME_OVER:
            if (key == SDLK_r) {
                save_result(g, sb);
                game_start_match(g);
            } else if (key == SDLK_m || key == SDLK_ESCAPE) {
                save_result(g, sb);
                g->state = STATE_MENU;
            }
            break;

        case STATE_SCOREBOARD:
            if (key == SDLK_ESCAPE || key == SDLK_m || key == SDLK_RETURN) {
                g->state = STATE_MENU;
            }
            break;

        default: break;
        }
    }

    if (event->type == SDL_KEYUP) {
        SDL_Keycode key = event->key.keysym.sym;
        if (key == SDLK_w)    key_w    = 0;
        if (key == SDLK_s)    key_s    = 0;
        if (key == SDLK_UP)   key_up   = 0;
        if (key == SDLK_DOWN) key_down = 0;
    }

    if (event->type == SDL_TEXTINPUT && g->state == STATE_NAME_INPUT) {
        int len = (int)strlen(event->text.text);
        if (g->input_len + len < 16) {
            strncat(g->input_buf, event->text.text, 15 - g->input_len);
            g->input_len += len;
        }
    }

    return 0;
}

void input_update_paddles(Game *g) {
    if (g->state != STATE_PLAYING && g->state != STATE_ROUND_OVER) return;

    /* P1 */
    if (g->mode == MODE_1P) {
        /* Modo 1P: W/S e setas funcionam */
        if ((key_w || key_up) && !(key_s || key_down))       g->p1.vy = -PADDLE_SPEED;
        else if ((key_s || key_down) && !(key_w || key_up))  g->p1.vy =  PADDLE_SPEED;
        else                                                  g->p1.vy = 0;
    } else {
        /* Modo 2P: apenas W/S para P1 */
        if (key_w && !key_s)       g->p1.vy = -PADDLE_SPEED;
        else if (key_s && !key_w)  g->p1.vy =  PADDLE_SPEED;
        else                       g->p1.vy = 0;
    }

    if (g->mode == MODE_2P) {
        if (key_up && !key_down)       g->p2.vy = -PADDLE_SPEED;
        else if (key_down && !key_up)  g->p2.vy =  PADDLE_SPEED;
        else                           g->p2.vy = 0;
    }
}
