#include "input.h"
#include "scoreboard.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Estado das teclas para movimento contínuo */
static int key_w = 0, key_s = 0;       /* P1 */
static int key_up = 0, key_down = 0;   /* P2 */

/* Salva resultado no scoreboard */
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

    /* Data atual */
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(entry.date, sizeof(entry.date), "%Y-%m-%d %H:%M", tm_info);

    scoreboard_insert(sb, &entry);
    scoreboard_save(sb, SCOREBOARD_FILE);
}

int input_handle(SDL_Event *event, Game *g, Scoreboard *sb) {
    if (event->type == SDL_QUIT) return 1;

    /* --- Eventos de teclado (keydown) --- */
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;

        /* Teclas de movimento — armazena estado */
        if (key == SDLK_w)      key_w    = 1;
        if (key == SDLK_s)      key_s    = 1;
        if (key == SDLK_UP)     key_up   = 1;
        if (key == SDLK_DOWN)   key_down = 1;

        switch (g->state) {

        /* --- MENU --- */
        case STATE_MENU:
            if (key == SDLK_UP || key == SDLK_w) {
                g->menu_sel = (g->menu_sel + 3) % 4;
            } else if (key == SDLK_DOWN || key == SDLK_s) {
                g->menu_sel = (g->menu_sel + 1) % 4;
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                switch (g->menu_sel) {
                case 0: /* 1P vs CPU */
                    g->mode = MODE_1P;
                    strcpy(g->p2_name, "CPU");
                    /* Vai para input de nome apenas P1 */
                    g->name_input_field = 0;
                    memset(g->input_buf, 0, sizeof(g->input_buf));
                    g->input_len = 0;
                    SDL_StartTextInput();
                    g->state = STATE_NAME_INPUT;
                    break;
                case 1: /* 2 jogadores */
                    g->mode = MODE_2P;
                    g->name_input_field = 0;
                    memset(g->input_buf, 0, sizeof(g->input_buf));
                    g->input_len = 0;
                    SDL_StartTextInput();
                    g->state = STATE_NAME_INPUT;
                    break;
                case 2: /* Scoreboard */
                    g->state = STATE_SCOREBOARD;
                    break;
                case 3: /* Sair */
                    return 1;
                }
            }
            break;

        /* --- INPUT DE NOMES --- */
        case STATE_NAME_INPUT:
            if (key == SDLK_BACKSPACE && g->input_len > 0) {
                g->input_buf[--g->input_len] = '\0';
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                /* Confirma campo atual */
                if (g->input_len == 0) {
                    /* Nome padrão */
                    if (g->name_input_field == 0) strcpy(g->input_buf, "Jogador 1");
                    else                           strcpy(g->input_buf, "Jogador 2");
                }
                if (g->name_input_field == 0) {
                    snprintf(g->p1_name, sizeof(g->p1_name), "%s", g->input_buf);
                    if (g->mode == MODE_2P) {
                        /* Vai para campo P2 */
                        g->name_input_field = 1;
                        memset(g->input_buf, 0, sizeof(g->input_buf));
                        g->input_len = 0;
                    } else {
                        /* Modo 1P — começa a partida */
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

        /* --- JOGANDO --- */
        case STATE_PLAYING:
            if (key == SDLK_p) {
                g->state = STATE_PAUSED;
            } else if (key == SDLK_ESCAPE) {
                g->state = STATE_MENU;
            }
            break;

        /* --- PAUSADO --- */
        case STATE_PAUSED:
            if (key == SDLK_p || key == SDLK_ESCAPE) {
                g->state = STATE_PLAYING;
            }
            break;

        /* --- FIM DE JOGO --- */
        case STATE_GAME_OVER:
            if (key == SDLK_r) {
                /* Salva resultado e joga novamente com mesmos nomes */
                save_result(g, sb);
                game_start_match(g);
            } else if (key == SDLK_m || key == SDLK_ESCAPE) {
                save_result(g, sb);
                g->state = STATE_MENU;
            }
            break;

        /* --- SCOREBOARD --- */
        case STATE_SCOREBOARD:
            if (key == SDLK_ESCAPE || key == SDLK_m || key == SDLK_RETURN) {
                g->state = STATE_MENU;
            }
            break;

        default: break;
        }
    }

    /* --- Solta teclas --- */
    if (event->type == SDL_KEYUP) {
        SDL_Keycode key = event->key.keysym.sym;
        if (key == SDLK_w)    key_w    = 0;
        if (key == SDLK_s)    key_s    = 0;
        if (key == SDLK_UP)   key_up   = 0;
        if (key == SDLK_DOWN) key_down = 0;
    }

    /* --- Texto digitado para input de nomes --- */
    if (event->type == SDL_TEXTINPUT && g->state == STATE_NAME_INPUT) {
        int len = (int)strlen(event->text.text);
        if (g->input_len + len < 16) {
            strncat(g->input_buf, event->text.text, 15 - g->input_len);
            g->input_len += len;
        }
    }

    return 0;
}

/* Atualiza velocidades dos paddles baseado nas teclas pressionadas */
void input_update_paddles(Game *g) {
    if (g->state != STATE_PLAYING && g->state != STATE_ROUND_OVER) return;

    /* P1 */
    if (key_w && !key_s)       g->p1.vy = -PADDLE_SPEED;
    else if (key_s && !key_w)  g->p1.vy =  PADDLE_SPEED;
    else                       g->p1.vy = 0;

    /* P2 (apenas no modo 2P) */
    if (g->mode == MODE_2P) {
        if (key_up && !key_down)       g->p2.vy = -PADDLE_SPEED;
        else if (key_down && !key_up)  g->p2.vy =  PADDLE_SPEED;
        else                           g->p2.vy = 0;
    }
}
