#include "render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void set_gray(SDL_Renderer *r)   { SDL_SetRenderDrawColor(r, 150, 150, 150, 255); }
static void set_yellow(SDL_Renderer *r) { SDL_SetRenderDrawColor(r, 255, 220,  60, 255); }

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h,
                      uint8_t R, uint8_t G, uint8_t B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(r, &rc);
}
#define PS 3   /* tamanho de cada pixel do sprite na tela */

static void ppx(SDL_Renderer *r, int gx, int gy, int ox, int oy,
                uint8_t R, uint8_t G, uint8_t B) {
    fill_rect(r, ox + gx*PS, oy + gy*PS, PS, PS, R, G, B);
}

static const uint8_t PENGUIN[22][10] = {
    {0,0,0,1,1,1,1,0,0,0},  
    {0,0,1,1,1,1,1,1,0,0},  
    {0,1,1,1,1,1,1,1,1,0},  
    {2,2,2,2,1,1,1,1,1,0},  
    {0,0,1,1,3,1,1,1,0,0},  
    {0,0,1,3,3,3,1,1,0,0},  
    {0,0,0,1,3,3,3,1,1,0},  
    {0,0,0,0,3,3,3,1,1,0},  
    {0,0,0,0,3,3,3,1,1,1},  
    {0,0,0,0,3,3,3,1,1,1},  
    {0,0,0,0,3,3,3,1,1,1},  
    {0,0,0,0,3,3,3,1,1,1},  
    {0,0,0,0,3,3,1,1,1,1},  
    {0,0,0,0,3,1,1,1,1,1},  
    {0,0,0,0,1,1,1,1,1,1},  
    {0,0,0,0,1,1,1,1,1,0}, 
    {0,0,0,1,1,1,1,1,1,0}, 
    {0,0,1,1,1,1,1,1,1,0}, 
    {0,0,1,1,1,1,1,1,0,0}, 
    {0,0,1,1,1,1,1,1,0,0}, 
    {0,0,1,2,2,2,2,2,0,0}, 
    {0,2,2,2,2,2,2,2,0,0}, 
};
#define SPRITE_COLS 10
#define SPRITE_ROWS 22

static void draw_penguin(SDL_Renderer *r, int x, int y,
                         SDL_Color body, int mirror) {
    int total_w = SPRITE_COLS * PS;
    int total_h = SPRITE_ROWS * PS;
    int ox = x + (PADDLE_W  - total_w) / 2;
    int oy = y + (PADDLE_H  - total_h) / 2;

    for (int row = 0; row < SPRITE_ROWS; row++) {
        for (int col = 0; col < SPRITE_COLS; col++) {
            int sc = mirror ? (SPRITE_COLS - 1 - col) : col;
            uint8_t v = PENGUIN[row][sc];
            if (v == 0) continue;
            uint8_t R, G, B;
            if      (v == 1) { R=body.r; G=body.g; B=body.b; }
            else if (v == 2) { R=255; G=126; B=0; }
            else             { R=240; G=240; B=240; } /* barriga */
            ppx(r, col, row, ox, oy, R, G, B);
        }
    }
}

#define BK 4   /* tamanho do bloco de paisagem */


static void bfill(SDL_Renderer *r, int x, int y, int w, int h,
                  uint8_t R, uint8_t G, uint8_t B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    int x0 = (x / BK) * BK;
    int y0 = (y / BK) * BK;
    int x1 = ((x+w+BK-1) / BK) * BK;
    int y1 = ((y+h+BK-1) / BK) * BK;
    SDL_Rect rc = { x0, y0, x1-x0, y1-y0 };
    SDL_RenderFillRect(r, &rc);
}

static void bline(SDL_Renderer *r, int y, uint8_t R, uint8_t G, uint8_t B) {
    int y0 = (y / BK) * BK;
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rc = { 0, y0, SCREEN_W, BK };
    SDL_RenderFillRect(r, &rc);
}

static unsigned int lcg(unsigned int s) {
    return s * 1664525u + 1013904223u;
}

static void draw_pixel_background(SDL_Renderer *r) {
    bfill(r, 0, 0, SCREEN_W, SCREEN_H/3, 8, 18, 60);
    bfill(r, 0, SCREEN_H/3, SCREEN_W, SCREEN_H/3, 14, 42, 100);
    int sky_h = SCREEN_H * 58 / 100; 
    bfill(r, 0, SCREEN_H*2/3, SCREEN_W, sky_h - SCREEN_H*2/3 + BK, 20, 70, 140);

    unsigned int seed = 42;
    for (int i = 0; i < 80; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy = (int)(seed % (unsigned)(sky_h * 55 / 100));
        seed = lcg(seed); int bright = (int)(seed % 3); 
        uint8_t lv = (bright == 2) ? 255 : (bright == 1) ? 180 : 110;
        int bx = (sx / BK) * BK;
        int by = (sy / BK) * BK;
        fill_rect(r, bx, by, BK, BK, lv, lv, lv);
    }

    {
        int y_start = 6*BK, y_end = 11*BK; 
        for (int bx = 0; bx < SCREEN_W; bx += BK) {
            int dy = (bx * BK) / (SCREEN_W / BK);
            for (int by = y_start; by < y_end; by += BK) {
                int fy = by + dy;
                if (fy >= 0 && fy < sky_h)
                    fill_rect(r, bx, fy, BK, BK, 30, 180, 160);
            }
        }
    }
    {
        int y_start = 11*BK, y_end = 15*BK;
        for (int bx = 0; bx < SCREEN_W; bx += BK) {
            int dy = (bx * BK) / (SCREEN_W / BK);
            for (int by = y_start; by < y_end; by += BK) {
                int fy = by + dy;
                if (fy >= 0 && fy < sky_h)
                    fill_rect(r, bx, fy, BK, BK, 20, 130, 200);
            }
        }
    }
    {
        int y_start = 14*BK, y_end = 18*BK;
        for (int bx = 0; bx < SCREEN_W; bx += BK) {
            int dy = (bx * BK) / (SCREEN_W / BK);
            for (int by = y_start; by < y_end; by += BK) {
                int fy = by + dy;
                if (fy >= 0 && fy < sky_h)
                    fill_rect(r, bx, fy, BK, BK, 90, 40, 180);
            }
        }
    }

    int mcols = SCREEN_W / BK;
    int peaks[] = { 0, mcols/6, mcols*2/6, mcols*3/6, mcols*4/6, mcols*5/6, mcols };
    int peak_h[] = { sky_h/BK, sky_h/BK - 8, sky_h/BK - 14,
                     sky_h/BK - 6,  sky_h/BK - 18,
                     sky_h/BK - 10, sky_h/BK };
    int num_peaks = 7;

    for (int bx = 0; bx < mcols; bx++) {
        int seg = 0;
        for (int p = 0; p < num_peaks - 1; p++) {
            if (bx >= peaks[p] && bx <= peaks[p+1]) { seg = p; break; }
        }
        float t = (peaks[seg+1] > peaks[seg])
                  ? (float)(bx - peaks[seg]) / (peaks[seg+1] - peaks[seg])
                  : 0.0f;
        int h_top = (int)(peak_h[seg] + t * (peak_h[seg+1] - peak_h[seg]));
        for (int by = h_top; by < sky_h/BK; by++) {
            uint8_t R, G, B;
            if (by == h_top) { R=200; G=230; B=248; }
            else             { R= 10; G= 40; B= 90; } 
            fill_rect(r, bx*BK, by*BK, BK, BK, R, G, B);
        }
    }

    bfill(r, 0, sky_h, SCREEN_W, (SCREEN_H - sky_h)*2/3,
          190, 230, 252);
    bfill(r, 0, sky_h + (SCREEN_H-sky_h)*2/3, SCREEN_W, SCREEN_H,
          220, 245, 255);

    seed = 137;
    for (int i = 0; i < 30; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy_off = (int)(seed % (unsigned)((SCREEN_H - sky_h)));
        int bx = (sx / BK) * BK;
        int by = ((sky_h + sy_off) / BK) * BK;
        seed = lcg(seed); int sw = BK * (int)(2 + seed % 5);
        fill_rect(r, bx, by, sw, BK, 140, 190, 230);
    }

    bline(r, sky_h, 255, 255, 255);

    seed = 999;
    for (int i = 0; i < 60; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy = (int)(seed % (unsigned)sky_h);
        int bx = (sx / BK) * BK;
        int by = (sy / BK) * BK;
        fill_rect(r, bx, by, BK, BK, 210, 235, 255);
    }
}

static void draw_text_centered(Renderer *r, TTF_Font *font,
                                const char *text, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, surf);
    SDL_Rect dst = { (SCREEN_W - surf->w) / 2, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    SDL_RenderCopy(r->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void draw_text(Renderer *r, TTF_Font *font,
                      const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    SDL_RenderCopy(r->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static void draw_center_line(Renderer *r) {
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r->renderer, 255, 255, 255, 120);
    int y = 0;
    while (y < SCREEN_H) {
        SDL_Rect rect = { SCREEN_W / 2 - 2, y, 4, 10 };
        SDL_RenderFillRect(r->renderer, &rect);
        y += 20;
    }
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_NONE);
}

static void draw_overlay(Renderer *r, uint8_t alpha) {
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r->renderer, 0, 10, 40, alpha);
    SDL_Rect full = { 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderFillRect(r->renderer, &full);
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_NONE);
}

int render_init(Renderer *r, SDL_Renderer *sdl_renderer) {
    r->renderer = sdl_renderer;
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        NULL
    };
    r->font_lg = r->font_md = r->font_sm = NULL;
    for (int i = 0; font_paths[i]; i++) {
        r->font_lg = TTF_OpenFont(font_paths[i], 48);
        if (r->font_lg) {
            r->font_md = TTF_OpenFont(font_paths[i], 28);
            r->font_sm = TTF_OpenFont(font_paths[i], 18);
            printf("Fonte carregada: %s\n", font_paths[i]);
            break;
        }
    }
    if (!r->font_lg) {
        fprintf(stderr, "Erro: nenhuma fonte encontrada. TTF erro: %s\n", TTF_GetError());
        return -1;
    }
    return 0;
}

void render_destroy(Renderer *r) {
    if (r->font_lg) TTF_CloseFont(r->font_lg);
    if (r->font_md) TTF_CloseFont(r->font_md);
    if (r->font_sm) TTF_CloseFont(r->font_sm);
}

static void render_menu(Renderer *r, const Game *g) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220,  60, 255};
    SDL_Color ice    = {180, 230, 255, 255};
    draw_text_centered(r, r->font_lg, "PINGO", 60, white);
    draw_text_centered(r, r->font_sm, "Aventura Gelada no Polo Sul", 120, ice);
    const char *items[] = {"1 Jogador vs CPU","2 Jogadores","Ver Scoreboard","Sair"};
    for (int i = 0; i < 4; i++) {
        int y = 220 + i * 70;
        SDL_Color col = (i == g->menu_sel) ? yellow : ice;
        if (i == g->menu_sel) {
            set_yellow(r->renderer);
            SDL_Rect mark = { SCREEN_W/2 - 160, y + 4, 8, 20 };
            SDL_RenderFillRect(r->renderer, &mark);
        }
        draw_text_centered(r, r->font_md, items[i], y, col);
    }
    draw_text_centered(r, r->font_sm, "Use setas + Enter para navegar", 540, ice);
}

static void render_name_input(Renderer *r, const Game *g) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220,  60, 255};
    SDL_Color ice    = {180, 230, 255, 255};
    draw_text_centered(r, r->font_md, "NOME DOS JOGADORES", 100, white);
    SDL_Color c1 = (g->name_input_field == 0) ? yellow : ice;
    draw_text_centered(r, r->font_sm, "Jogador 1 (W/S):", 200, ice);
    char buf1[48];
    snprintf(buf1, sizeof(buf1), (g->name_input_field==0) ? "%s_" : "%s",
             (g->name_input_field==0) ? g->input_buf : g->p1_name);
    draw_text_centered(r, r->font_md, buf1, 228, c1);
    if (g->mode == MODE_2P) {
        SDL_Color c2 = (g->name_input_field == 1) ? yellow : ice;
        draw_text_centered(r, r->font_sm, "Jogador 2 (setas):", 310, ice);
        char buf2[48];
        snprintf(buf2, sizeof(buf2), (g->name_input_field==1) ? "%s_" : "%s",
                 (g->name_input_field==1) ? g->input_buf : g->p2_name);
        draw_text_centered(r, r->font_md, buf2, 338, c2);
    }
    draw_text_centered(r, r->font_sm, "Enter para confirmar | Backspace para apagar", 480, ice);
}

static void render_playing(Renderer *r, const Game *g) {
    SDL_Color white = {255, 255, 255, 255};
    draw_center_line(r);

    SDL_Color p1_body = {15, 15, 15, 255};
    draw_penguin(r->renderer, (int)g->p1.rect.x, (int)g->p1.rect.y, p1_body, 1);
    SDL_Color p2_body = {20, 55, 140, 255};
    draw_penguin(r->renderer, (int)g->p2.rect.x, (int)g->p2.rect.y, p2_body, 0);

    SDL_SetRenderDrawColor(r->renderer, 255, 255, 255, 255);
    SDL_Rect rb = { (int)g->ball.rect.x, (int)g->ball.rect.y,
                    (int)g->ball.rect.w, (int)g->ball.rect.h };
    SDL_RenderFillRect(r->renderer, &rb);

    char sc[32];
    snprintf(sc, sizeof(sc), "%d", g->p1.score);
    draw_text(r, r->font_lg, sc, SCREEN_W/2 - 80, 20, white);
    snprintf(sc, sizeof(sc), "%d", g->p2.score);
    draw_text(r, r->font_lg, sc, SCREEN_W/2 + 40, 20, white);

    SDL_Color ice = {180, 230, 255, 255};
    draw_text(r, r->font_sm, g->p1_name, PADDLE_MARGIN, SCREEN_H - 30, ice);
    int tw = 0;
    TTF_SizeUTF8(r->font_sm, g->p2_name, &tw, NULL);
    draw_text(r, r->font_sm, g->p2_name, SCREEN_W - PADDLE_MARGIN - tw, SCREEN_H - 30, ice);
}

static void render_paused(Renderer *r, const Game *g) {
    render_playing(r, g);
    draw_overlay(r, 140);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color ice   = {180, 230, 255, 255};
    draw_text_centered(r, r->font_lg, "PAUSADO", SCREEN_H/2 - 50, white);
    draw_text_centered(r, r->font_sm, "Pressione P para continuar", SCREEN_H/2 + 30, ice);
}

static void render_round_over(Renderer *r, const Game *g) {
    render_playing(r, g);
    draw_overlay(r, 100);
    SDL_Color white = {255, 255, 255, 255};
    char msg[64];
    snprintf(msg, sizeof(msg), "%d  x  %d", g->p1.score, g->p2.score);
    draw_text_centered(r, r->font_lg, msg, SCREEN_H/2 - 40, white);
}

static void render_game_over(Renderer *r, const Game *g) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220,  60, 255};
    SDL_Color ice    = {180, 230, 255, 255};
    draw_text_centered(r, r->font_lg, "FIM DE JOGO", 80, white);
    char msg[128];
    const char *winner = (g->p1.score > g->p2.score) ? g->p1_name : g->p2_name;
    snprintf(msg, sizeof(msg), "%s venceu!", winner);
    draw_text_centered(r, r->font_md, msg, 180, yellow);
    snprintf(msg, sizeof(msg), "%s  %d  x  %d  %s",
             g->p1_name, g->p1.score, g->p2.score, g->p2_name);
    draw_text_centered(r, r->font_md, msg, 240, white);
    draw_text_centered(r, r->font_sm, "R - Jogar novamente", 360, ice);
    draw_text_centered(r, r->font_sm, "M - Voltar ao menu",  400, ice);
    draw_text_centered(r, r->font_sm, "Resultado salvo no scoreboard!", 460, ice);
}

static void render_scoreboard(Renderer *r, const Scoreboard *sb) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 220,  60, 255};
    SDL_Color ice    = {180, 230, 255, 255};
    draw_text_centered(r, r->font_md, "SCOREBOARD", 30, white);
    if (sb->count == 0) {
        draw_text_centered(r, r->font_sm, "Nenhuma partida registrada ainda.", 220, ice);
    } else {
        draw_text(r, r->font_sm, "#",          50,  90, ice);
        draw_text(r, r->font_sm, "Vencedor",   80,  90, ice);
        draw_text(r, r->font_sm, "Adversario", 280, 90, ice);
        draw_text(r, r->font_sm, "Placar",     460, 90, ice);
        draw_text(r, r->font_sm, "Data",       570, 90, ice);
        set_gray(r->renderer);
        SDL_RenderDrawLine(r->renderer, 40, 112, SCREEN_W - 40, 112);
        for (int i = 0; i < sb->count && i < 10; i++) {
            const ScoreEntry *e = &sb->entries[i];
            int y = 124 + i * 40;
            SDL_Color nc = (i == 0) ? yellow : white;
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            draw_text(r, r->font_sm, num, 50, y, nc);
            draw_text(r, r->font_sm, e->winner, 80, y, nc);
            const char *loser = (strcmp(e->winner, e->p1_name)==0) ? e->p2_name : e->p1_name;
            draw_text(r, r->font_sm, loser, 280, y, ice);
            char sc[16];
            snprintf(sc, sizeof(sc), "%d x %d", e->score_p1, e->score_p2);
            draw_text(r, r->font_sm, sc, 460, y, white);
            draw_text(r, r->font_sm, e->date, 570, y, ice);
        }
    }
    draw_text_centered(r, r->font_sm, "Pressione Esc ou M para voltar", 555, ice);
}

void render_frame(Renderer *r, const Game *g, const Scoreboard *sb) {
    draw_pixel_background(r->renderer);

    switch (g->state) {
        case STATE_MENU:        render_menu(r, g);        break;
        case STATE_NAME_INPUT:  render_name_input(r, g);  break;
        case STATE_PLAYING:     render_playing(r, g);     break;
        case STATE_PAUSED:      render_paused(r, g);      break;
        case STATE_ROUND_OVER:  render_round_over(r, g);  break;
        case STATE_GAME_OVER:   render_game_over(r, g);   break;
        case STATE_SCOREBOARD:  render_scoreboard(r, sb); break;
    }

    SDL_RenderPresent(r->renderer);
}
