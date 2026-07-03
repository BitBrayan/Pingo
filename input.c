#include "input.h"
#include "scoreboard.h"
#include "audio.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Estado das teclas para movimento contínuo */
static int key_w = 0, key_s = 0;       /* P1 */
static int key_up = 0, key_down = 0;   /* P2 */

static const struct {
    int w, h;
    const char *label;
} resolutions[] = {
    {800, 600, "800x600"},
    {1024, 768, "1024x768"},
    {1280, 720, "1280x720"},
    {1366, 768, "1366x768"}
};
static const int resolution_count = sizeof(resolutions) / sizeof(resolutions[0]);

static void window_to_logical(SDL_Window *window, int wx, int wy, int *lx, int *ly) {
    int ww = 0, wh = 0;
    SDL_GetWindowSize(window, &ww, &wh);
    if (ww <= 0 || wh <= 0) {
        *lx = wx;
        *ly = wy;
        return;
    }
    *lx = wx * SCREEN_W / ww;
    *ly = wy * SCREEN_H / wh;
}

static SDL_Rect menu_item_rect(int index) {
    return (SDL_Rect){ SCREEN_W/2 - 220, 220 + index * 70 - 10, 440, 50 };
}

static SDL_Rect options_item_rect(int index) {
    return (SDL_Rect){ SCREEN_W/2 - 220, 220 + index * 70 - 10, 620, 50 };
}

static void clamp_volume(int *value) {
    if (*value < 0) *value = 0;
    if (*value > 100) *value = 100;
}

static void apply_resolution(SDL_Window *window, const Game *g) {
    if (!g) return;

    if (g->resolution_idx < 0 || g->resolution_idx >= resolution_count) return;

    SDL_DisplayMode mode;
    memset(&mode, 0, sizeof(mode));
    mode.w = resolutions[g->resolution_idx].w;
    mode.h = resolutions[g->resolution_idx].h;

    if (g->video_mode == VIDEO_FULLSCREEN) {
        if (SDL_SetWindowDisplayMode(window, &mode) != 0) {
            fprintf(stderr, "Erro ao definir modo de vídeo %dx%d: %s\n",
                    mode.w, mode.h, SDL_GetError());
            return;
        }
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
            fprintf(stderr, "Erro ao entrar em fullscreen na resolução %dx%d: %s\n",
                    mode.w, mode.h, SDL_GetError());
            return;
        }
        SDL_SetWindowSize(window, resolutions[g->resolution_idx].w, resolutions[g->resolution_idx].h);
    } else {
        if (SDL_SetWindowFullscreen(window, 0) != 0) {
            fprintf(stderr, "Erro ao sair do fullscreen: %s\n", SDL_GetError());
            return;
        }
        SDL_SetWindowDisplayMode(window, NULL);
        SDL_SetWindowSize(window, resolutions[g->resolution_idx].w, resolutions[g->resolution_idx].h);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

static void apply_option_change(Game *g, SDL_Window *window, int button) {
    if (g->options_sel == 0) {
        g->video_mode = (g->video_mode == VIDEO_FULLSCREEN) ? VIDEO_WINDOWED : VIDEO_FULLSCREEN;
        apply_resolution(window, g);
        audio_play_sfx(SFX_MENU);
    } else if (g->options_sel == 1) {
        if (button == SDL_BUTTON_RIGHT) {
            g->resolution_idx = (g->resolution_idx + resolution_count - 1) % resolution_count;
        } else {
            g->resolution_idx = (g->resolution_idx + 1) % resolution_count;
        }
        apply_resolution(window, g);
        audio_play_sfx(SFX_MENU);
    } else if (g->options_sel == 2) {
        if (button == SDL_BUTTON_RIGHT) g->music_volume -= 10;
        else g->music_volume += 10;
        clamp_volume(&g->music_volume);
        audio_set_music_volume(g->music_volume);
        audio_play_sfx(SFX_MENU);
    } else if (g->options_sel == 3) {
        if (button == SDL_BUTTON_RIGHT) g->sfx_volume -= 10;
        else g->sfx_volume += 10;
        clamp_volume(&g->sfx_volume);
        audio_set_sfx_volume(g->sfx_volume);
        audio_play_sfx(SFX_MENU);
    } else if (g->options_sel == 4) {
        g->state = STATE_MENU;
        audio_play_sfx(SFX_MENU);
    }
}

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

static int activate_menu_item(Game *g) {
    switch (g->menu_sel) {
    case 0: /* 1P vs CPU */
        g->mode = MODE_1P;
        strcpy(g->p2_name, "CPU");
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
    case 2: /* Opções */
        g->options_sel = 0;
        g->state = STATE_OPTIONS;
        audio_play_sfx(SFX_MENU);
        break;
    case 3: /* Scoreboard */
        g->state = STATE_SCOREBOARD;
        audio_play_sfx(SFX_MENU);
        break;
    case 4: /* Sair */
        return 1;
    }
    return 0;
}

int input_handle(SDL_Event *event, Game *g, Scoreboard *sb, SDL_Window *window) {
    if (event->type == SDL_QUIT) return 1;

    /* --- Eventos do mouse --- */
    if (event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEBUTTONDOWN) {
        int mx, my;
        int button = 0;
        if (event->type == SDL_MOUSEMOTION) {
            mx = event->motion.x;
            my = event->motion.y;
        } else {
            mx = event->button.x;
            my = event->button.y;
            button = event->button.button;
        }
        int lx, ly;
        window_to_logical(window, mx, my, &lx, &ly);
        SDL_Point pt = { lx, ly };

        if (g->state == STATE_MENU) {
            for (int i = 0; i < 5; i++) {
                SDL_Rect rect = menu_item_rect(i);
                if (SDL_PointInRect(&pt, &rect)) {
                    if (g->menu_sel != i) {
                        g->menu_sel = i;
                        audio_play_sfx(SFX_MENU);
                    }
                    if (event->type == SDL_MOUSEBUTTONDOWN && button == SDL_BUTTON_LEFT) {
                        if (activate_menu_item(g)) return 1;
                    }
                    break;
                }
            }
        } else if (g->state == STATE_OPTIONS) {
            for (int i = 0; i < 5; i++) {
                SDL_Rect rect = options_item_rect(i);
                if (SDL_PointInRect(&pt, &rect)) {
                    if (g->options_sel != i) {
                        g->options_sel = i;
                        audio_play_sfx(SFX_MENU);
                    }
                    if (event->type == SDL_MOUSEBUTTONDOWN && (button == SDL_BUTTON_LEFT || button == SDL_BUTTON_RIGHT)) {
                        apply_option_change(g, window, button);
                    }
                    break;
                }
            }
        }
    }

    /* --- Eventos de teclado (keydown) --- */
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

        /* --- MENU --- */
        case STATE_MENU:
            if (key == SDLK_UP || key == SDLK_w) {
                g->menu_sel = (g->menu_sel + 4) % 5;
                audio_play_sfx(SFX_MENU);
            } else if (key == SDLK_DOWN || key == SDLK_s) {
                g->menu_sel = (g->menu_sel + 1) % 5;
                audio_play_sfx(SFX_MENU);
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                if (activate_menu_item(g)) return 1;
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

        /* --- OPÇÕES --- */
        case STATE_OPTIONS:
            if (key == SDLK_UP || key == SDLK_w) {
                g->options_sel = (g->options_sel + 4) % 5;
                audio_play_sfx(SFX_MENU);
            } else if (key == SDLK_DOWN || key == SDLK_s) {
                g->options_sel = (g->options_sel + 1) % 5;
                audio_play_sfx(SFX_MENU);
            } else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
                int dir = (key == SDLK_LEFT) ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
                apply_option_change(g, window, dir);
            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                apply_option_change(g, window, SDL_BUTTON_LEFT);
            } else if (key == SDLK_ESCAPE) {
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

    /* P2 (apenas no modo 2P) */
    if (g->mode == MODE_2P) {
        if (key_up && !key_down)       g->p2.vy = -PADDLE_SPEED;
        else if (key_down && !key_up)  g->p2.vy =  PADDLE_SPEED;
        else                           g->p2.vy = 0;
    }
}
