#include "render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * UI palette (cores padronizadas)
 * --------------------------------------------------------------------- */
static const SDL_Color COLOR_WHITE      = {255, 255, 255, 255}; /* #FFFFFF */
static const SDL_Color COLOR_GOLD       = {255, 213,  74, 255}; /* #FFD54A */
static const SDL_Color COLOR_ICE        = {223, 244, 255, 255}; /* #DFF4FF */
static const SDL_Color COLOR_NAVY       = { 41,  78, 142, 255}; /* #294E8E */
static const SDL_Color COLOR_NAVY_DARK  = { 22,  33,  62, 255}; /* #16213E */
static const SDL_Color COLOR_DISABLED   = {143, 168, 201, 255}; /* #8FA8C9 */
static const SDL_Color PANEL_BG         = { 20,  42,  90, 170}; /* #142A5A + alpha */


/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */
static void set_gray(SDL_Renderer *r)   { SDL_SetRenderDrawColor(r, 150, 150, 150, 255); }
static void set_yellow(SDL_Renderer *r) { SDL_SetRenderDrawColor(r, 255, 220,  60, 255); }

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h,
                      uint8_t R, uint8_t G, uint8_t B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(r, &rc);
}

/* Fill with alpha (blended) */
static void fill_rect_alpha(SDL_Renderer *r, int x, int y, int w, int h,
                           uint8_t R, uint8_t G, uint8_t B, uint8_t A) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(r, &rc);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* Desenha um "pixel" da pixel-art com tamanho PS×PS */
#define PS 3   /* tamanho de cada pixel do sprite na tela */

static void ppx(SDL_Renderer *r, int gx, int gy, int ox, int oy,
                uint8_t R, uint8_t G, uint8_t B) {
    fill_rect(r, ox + gx*PS, oy + gy*PS, PS, PS, R, G, B);
}

/* -----------------------------------------------------------------------
 * Sprite 10×22 — extraído pixel a pixel da referência
 * 0=transparente 1=corpo 2=laranja 3=barriga branca
 * --------------------------------------------------------------------- */
static const uint8_t PENGUIN[22][10] = {
    {0,0,0,1,1,1,1,0,0,0},  /*  0 topo cabeça */
    {0,0,1,1,1,1,1,1,0,0},  /*  1 */
    {0,1,1,1,1,1,1,1,1,0},  /*  2 */
    {2,2,2,2,1,1,1,1,1,0},  /*  3 bico (esquerda) */
    {0,0,1,1,3,1,1,1,0,0},  /*  4 início barriga */
    {0,0,1,3,3,3,1,1,0,0},  /*  5 */
    {0,0,0,1,3,3,3,1,1,0},  /*  6 */
    {0,0,0,0,3,3,3,1,1,0},  /*  7 */
    {0,0,0,0,3,3,3,1,1,1},  /*  8 */
    {0,0,0,0,3,3,3,1,1,1},  /*  9 */
    {0,0,0,0,3,3,3,1,1,1},  /* 10 */
    {0,0,0,0,3,3,3,1,1,1},  /* 11 */
    {0,0,0,0,3,3,1,1,1,1},  /* 12 */
    {0,0,0,0,3,1,1,1,1,1},  /* 13 fim barriga */
    {0,0,0,0,1,1,1,1,1,1},  /* 14 */
    {0,0,0,0,1,1,1,1,1,0},  /* 15 */
    {0,0,0,1,1,1,1,1,1,0},  /* 16 */
    {0,0,1,1,1,1,1,1,1,0},  /* 17 */
    {0,0,1,1,1,1,1,1,0,0},  /* 18 */
    {0,0,1,1,1,1,1,1,0,0},  /* 19 */
    {0,0,1,2,2,2,2,2,0,0},  /* 20 pés */
    {0,2,2,2,2,2,2,2,0,0},  /* 21 pés */
};
#define SPRITE_COLS 10
#define SPRITE_ROWS 22

/* draw_penguin
 * x,y    = canto superior esquerdo da área PADDLE_W × PADDLE_H
 * body   = cor do corpo
 * mirror = 1 → espelha horizontalmente (P1 enfrenta direita)
 *          0 → original            (P2 enfrenta esquerda) */
static void draw_penguin(SDL_Renderer *r, int x, int y,
                         SDL_Color body, int mirror) {
    /* Centralizar sprite na área do paddle */
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

/* -----------------------------------------------------------------------
 * Paisagem pixelada
 *
 * Tudo desenhado com blocos de tamanho BK×BK para look pixel-art.
 * BK=4: céu, estrelas, aurora, montanhas, neve — tudo em blocos.
 * --------------------------------------------------------------------- */
#define BK 4   /* tamanho do bloco de paisagem */

/* Preenchimento em blocos */
static void bfill(SDL_Renderer *r, int x, int y, int w, int h,
                  uint8_t R, uint8_t G, uint8_t B) {
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    /* Snap para grid de BK */
    int x0 = (x / BK) * BK;
    int y0 = (y / BK) * BK;
    int x1 = ((x+w+BK-1) / BK) * BK;
    int y1 = ((y+h+BK-1) / BK) * BK;
    SDL_Rect rc = { x0, y0, x1-x0, y1-y0 };
    SDL_RenderFillRect(r, &rc);
}

/* Uma "linha" horizontal pixelada (espessura BK) */
static void bline(SDL_Renderer *r, int y, uint8_t R, uint8_t G, uint8_t B) {
    int y0 = (y / BK) * BK;
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_Rect rc = { 0, y0, SCREEN_W, BK };
    SDL_RenderFillRect(r, &rc);
}

/* LCG simples para estrelas determinísticas */
static unsigned int lcg(unsigned int s) {
    return s * 1664525u + 1013904223u;
}

static void draw_pixel_background(SDL_Renderer *r) {
    /* --- Céu: blocos de cor, 3 faixas de degradê --- */
    /* Faixa 1: azul escuro (topo) */
    bfill(r, 0, 0, SCREEN_W, SCREEN_H/3, 8, 18, 60);
    /* Faixa 2: azul médio */
    bfill(r, 0, SCREEN_H/3, SCREEN_W, SCREEN_H/3, 14, 42, 100);
    /* Faixa 3 (céu→horizonte): azul mais claro */
    int sky_h = SCREEN_H * 58 / 100; /* horizonte a 58% */
    bfill(r, 0, SCREEN_H*2/3, SCREEN_W, sky_h - SCREEN_H*2/3 + BK, 20, 70, 140);

    /* --- Estrelas (blocos BK×BK) --- */
    unsigned int seed = 42;
    for (int i = 0; i < 80; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy = (int)(seed % (unsigned)(sky_h * 55 / 100));
        seed = lcg(seed); int bright = (int)(seed % 3); /* 0=dim 1=med 2=bright */
        uint8_t lv = (bright == 2) ? 255 : (bright == 1) ? 180 : 110;
        /* Snap para grid */
        int bx = (sx / BK) * BK;
        int by = (sy / BK) * BK;
        fill_rect(r, bx, by, BK, BK, lv, lv, lv);
    }

    /* --- Aurora boreal: faixas diagonais em blocos --- */
    /* Cada faixa é desenhada como blocos BK×BK ao longo de uma diagonal */
    /* Verde-azulado */
    {
        int y_start = 6*BK, y_end = 11*BK; /* em pixels */
        for (int bx = 0; bx < SCREEN_W; bx += BK) {
            /* deslocamento vertical proporcional à posição x */
            int dy = (bx * BK) / (SCREEN_W / BK);
            for (int by = y_start; by < y_end; by += BK) {
                int fy = by + dy;
                if (fy >= 0 && fy < sky_h)
                    fill_rect(r, bx, fy, BK, BK, 30, 180, 160);
            }
        }
    }
    /* Azul-ciano */
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
    /* Violeta */
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

    /* --- Montanhas de gelo pixeladas ao fundo --- */
    /* Definidas como perfil de altura por bloco */
    /* Altura (em blocos BK) para cada coluna de blocos */
    int mcols = SCREEN_W / BK;
    /* Perfil manual simplificado: 5 picos distribuídos */
    /* Usamos uma função que gera um perfil "dente de serra" */
    int peaks[] = { 0, mcols/6, mcols*2/6, mcols*3/6, mcols*4/6, mcols*5/6, mcols };
    int peak_h[] = { sky_h/BK, sky_h/BK - 8, sky_h/BK - 14,
                     sky_h/BK - 6,  sky_h/BK - 18,
                     sky_h/BK - 10, sky_h/BK };
    int num_peaks = 7;

    for (int bx = 0; bx < mcols; bx++) {
        /* Interpolar entre picos */
        int seg = 0;
        for (int p = 0; p < num_peaks - 1; p++) {
            if (bx >= peaks[p] && bx <= peaks[p+1]) { seg = p; break; }
        }
        float t = (peaks[seg+1] > peaks[seg])
                  ? (float)(bx - peaks[seg]) / (peaks[seg+1] - peaks[seg])
                  : 0.0f;
        int h_top = (int)(peak_h[seg] + t * (peak_h[seg+1] - peak_h[seg]));
        /* Coluna de montanha do topo h_top até sky_h */
        for (int by = h_top; by < sky_h/BK; by++) {
            uint8_t R, G, B;
            if (by == h_top) { R=200; G=230; B=248; } /* neve no topo */
            else             { R= 10; G= 40; B= 90; } /* azul escuro */
            fill_rect(r, bx*BK, by*BK, BK, BK, R, G, B);
        }
    }

    /* --- Chão de neve pixelado --- */
    /* Faixa 1: neve azul-clara */
    bfill(r, 0, sky_h, SCREEN_W, (SCREEN_H - sky_h)*2/3,
          190, 230, 252);
    /* Faixa 2: neve mais branca (perto da câmera) */
    bfill(r, 0, sky_h + (SCREEN_H-sky_h)*2/3, SCREEN_W, SCREEN_H,
          220, 245, 255);

    /* Sombras na neve: blocos azuis escuros espalhados */
    seed = 137;
    for (int i = 0; i < 30; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy_off = (int)(seed % (unsigned)((SCREEN_H - sky_h)));
        int bx = (sx / BK) * BK;
        int by = ((sky_h + sy_off) / BK) * BK;
        seed = lcg(seed); int sw = BK * (int)(2 + seed % 5);
        fill_rect(r, bx, by, sw, BK, 140, 190, 230);
    }

    /* Linha de horizonte (neve brilhante) */
    bline(r, sky_h, 255, 255, 255);

    /* Flocos de neve: blocos BK×BK espalhados */
    seed = 999;
    for (int i = 0; i < 60; i++) {
        seed = lcg(seed); int sx = (int)(seed % (unsigned)SCREEN_W);
        seed = lcg(seed); int sy = (int)(seed % (unsigned)sky_h);
        int bx = (sx / BK) * BK;
        int by = (sy / BK) * BK;
        fill_rect(r, bx, by, BK, BK, 210, 235, 255);
    }
}

/* -----------------------------------------------------------------------
 * Texto helpers
 * --------------------------------------------------------------------- */
static void draw_text_centered(Renderer *r, TTF_Font *font,
                                const char *text, int y, SDL_Color color) {
    if (!text || !font) return;
    /* Sombra (deslocamento 2,2) */
    SDL_Color shadow = COLOR_NAVY_DARK;
    SDL_Surface *s_sh = TTF_RenderUTF8_Blended(font, text, shadow);
    if (s_sh) {
        SDL_Texture *t_sh = SDL_CreateTextureFromSurface(r->renderer, s_sh);
        if (t_sh) {
            SDL_Rect dst_sh = { (SCREEN_W - s_sh->w) / 2 + 2, y + 2, s_sh->w, s_sh->h };
            SDL_RenderCopy(r->renderer, t_sh, NULL, &dst_sh);
            SDL_DestroyTexture(t_sh);
        }
        SDL_FreeSurface(s_sh);
    }
    /* Texto principal */
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r->renderer, s);
    if (t) {
        SDL_Rect dst = { (SCREEN_W - s->w) / 2, y, s->w, s->h };
        SDL_RenderCopy(r->renderer, t, NULL, &dst);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

static void draw_text(Renderer *r, TTF_Font *font,
                      const char *text, int x, int y, SDL_Color color) {
    if (!text || !font) return;
    SDL_Color shadow = COLOR_NAVY_DARK;
    SDL_Surface *s_sh = TTF_RenderUTF8_Blended(font, text, shadow);
    if (s_sh) {
        SDL_Texture *t_sh = SDL_CreateTextureFromSurface(r->renderer, s_sh);
        if (t_sh) {
            SDL_Rect dst_sh = { x + 2, y + 2, s_sh->w, s_sh->h };
            SDL_RenderCopy(r->renderer, t_sh, NULL, &dst_sh);
            SDL_DestroyTexture(t_sh);
        }
        SDL_FreeSurface(s_sh);
    }
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r->renderer, s);
    if (t) {
        SDL_Rect dst = { x, y, s->w, s->h };
        SDL_RenderCopy(r->renderer, t, NULL, &dst);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
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

/* -----------------------------------------------------------------------
 * render_init / render_destroy
 * --------------------------------------------------------------------- */
static void update_renderer_scale(Renderer *r) {
    int ww = 0, wh = 0;
    SDL_GetRendererOutputSize(r->renderer, &ww, &wh);
    if (ww > 0 && wh > 0) {
        float sx = (float)ww / SCREEN_W;
        float sy = (float)wh / SCREEN_H;
        SDL_RenderSetScale(r->renderer, sx, sy);
    }
}

int render_init(Renderer *r, SDL_Renderer *sdl_renderer) {
    r->renderer = sdl_renderer;

     /* Usa escala manual do renderer para preencher a resolução/janela escolhida.
         SDL_RenderSetLogicalSize preserva a proporção e pode adicionar barras pretas,
         então escalamos o conteúdo diretamente. */

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

/* -----------------------------------------------------------------------
 * Telas
 * --------------------------------------------------------------------- */
static void render_menu(Renderer *r, const Game *g) {
    draw_text_centered(r, r->font_lg, "PINGO", 60, COLOR_WHITE);
    draw_text_centered(r, r->font_sm, "Aventura Gelada no Polo Sul", 120, COLOR_ICE);
    const char *items[] = {"1 Jogador vs CPU","2 Jogadores","Opções","Ver Scoreboard","Sair"};
    for (int i = 0; i < 5; i++) {
        int y = 220 + i * 70;
        if (i == g->menu_sel) {
            /* Desenha o cursor e o destaque */
            draw_text(r, r->font_md, ">", SCREEN_W/2 - 220, y, COLOR_GOLD);
            draw_text_centered(r, r->font_md, items[i], y, COLOR_GOLD);
        } else {
            draw_text_centered(r, r->font_md, items[i], y, COLOR_WHITE);
        }
    }
    draw_text_centered(r, r->font_sm, "Use mouse, setas + Enter para navegar", 540, COLOR_NAVY);
}

static void draw_custom_cursor(Renderer *r) {
    int mx, my;
    int ww = 0, wh = 0;
    SDL_GetMouseState(&mx, &my);
    SDL_GetRendererOutputSize(r->renderer, &ww, &wh);
    if (ww <= 0 || wh <= 0) return;
    float fx = mx * (float)SCREEN_W / ww;
    float fy = my * (float)SCREEN_H / wh;
    int x = (int)(fx + 0.5f);
    int y = (int)(fy + 0.5f);

    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r->renderer, COLOR_GOLD.r, COLOR_GOLD.g, COLOR_GOLD.b, 200);
    SDL_Rect center = { x - 6, y - 6, 12, 12 };
    SDL_RenderFillRect(r->renderer, &center);
    SDL_SetRenderDrawColor(r->renderer, COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b, 255);
    SDL_RenderDrawLine(r->renderer, x - 12, y, x + 12, y);
    SDL_RenderDrawLine(r->renderer, x, y - 12, x, y + 12);
    SDL_RenderDrawLine(r->renderer, x - 8, y - 8, x + 8, y + 8);
    SDL_RenderDrawLine(r->renderer, x - 8, y + 8, x + 8, y - 8);
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_NONE);
}

static void render_options_menu(Renderer *r, const Game *g) {
    static const char *res_labels[] = {"800x600", "1024x768", "1280x720", "1366x768"};
    draw_text_centered(r, r->font_lg, "OPÇÕES", 60, COLOR_WHITE);
    draw_text_centered(r, r->font_sm, "Ajuste resolução e volumes do jogo", 120, COLOR_ICE);

    const char *labels[] = {"Modo vídeo:", "Resolução atual:", "Volume Música:", "Volume SFX:", "Voltar"};
    for (int i = 0; i < 5; i++) {
        int y = 220 + i * 70;
        SDL_Color color = (g->options_sel == i) ? COLOR_GOLD : COLOR_WHITE;
        draw_text(r, r->font_md, labels[i], SCREEN_W/2 - 220, y, color);

        if (i == 0) {
            const char *video_label = (g->video_mode == VIDEO_FULLSCREEN) ? "Tela cheia" : "Janela";
            char buf[32];
            snprintf(buf, sizeof(buf), "< %s >", video_label);
            draw_text(r, r->font_md, buf, SCREEN_W/2 + 220, y, color);
        } else if (i == 1) {
            char buf[32];
            snprintf(buf, sizeof(buf), "< %s >", res_labels[g->resolution_idx]);
            draw_text(r, r->font_md, buf, SCREEN_W/2 + 220, y, color);
        } else if (i == 2) {
            char buf[16]; snprintf(buf, sizeof(buf), "%d%%", g->music_volume);
            draw_text(r, r->font_md, buf, SCREEN_W/2 + 220, y, color);
        } else if (i == 3) {
            char buf[16]; snprintf(buf, sizeof(buf), "%d%%", g->sfx_volume);
            draw_text(r, r->font_md, buf, SCREEN_W/2 + 220, y, color);
        }
    }

    draw_text_centered(r, r->font_sm, "Clique e use as setas para alterar", 585, COLOR_NAVY);
}

static void render_name_input(Renderer *r, const Game *g) {
    /* Panel behind the name input to guarantee contrast */
    int panel_w = SCREEN_W - 160;
    int panel_h = 360;
    int panel_x = (SCREEN_W - panel_w) / 2;
    int panel_y = (SCREEN_H - panel_h) / 2 - 20;
    fill_rect_alpha(r->renderer, panel_x, panel_y, panel_w, panel_h,
                    PANEL_BG.r, PANEL_BG.g, PANEL_BG.b, PANEL_BG.a);

    /* Spacing inside panel */
    int top = panel_y + 24;
    int gap = 54;
    draw_text_centered(r, r->font_md, "NOME DOS JOGADORES", top, COLOR_WHITE);
    draw_text_centered(r, r->font_sm, "Primeiro a alcançar 7 pontos vence!", top + gap, COLOR_ICE);

    int field_y = top + gap*2 + 10;
    SDL_Color c1 = (g->name_input_field == 0) ? COLOR_GOLD : COLOR_ICE;
    draw_text_centered(r, r->font_sm, "Jogador 1 (W/S):", field_y, COLOR_ICE);
    char buf1[48];
    snprintf(buf1, sizeof(buf1), (g->name_input_field==0) ? "%s_" : "%s",
             (g->name_input_field==0) ? g->input_buf : g->p1_name);
    draw_text_centered(r, r->font_md, buf1, field_y + 28, c1);

    if (g->mode == MODE_2P) {
        int field2_y = field_y + 90;
        SDL_Color c2 = (g->name_input_field == 1) ? COLOR_GOLD : COLOR_ICE;
        draw_text_centered(r, r->font_sm, "Jogador 2 (W/S):", field2_y, COLOR_ICE);
        char buf2[48];
        snprintf(buf2, sizeof(buf2), (g->name_input_field==1) ? "%s_" : "%s",
                 (g->name_input_field==1) ? g->input_buf : g->p2_name);
        draw_text_centered(r, r->font_md, buf2, field2_y + 28, c2);
    }

    draw_text_centered(r, r->font_sm, "Enter para confirmar | Backspace para apagar", panel_y + panel_h - 36, COLOR_NAVY);
}

static void render_playing(Renderer *r, const Game *g) {
    SDL_Color white = COLOR_WHITE;
    draw_center_line(r);

    /* P1 esquerda: espelhado → bico aponta para direita (enfrenta campo) */
    SDL_Color p1_body = {15, 15, 15, 255};
    draw_penguin(r->renderer, (int)g->p1.rect.x, (int)g->p1.rect.y, p1_body, 1);

    /* P2 direita: original → bico aponta para esquerda (enfrenta P1) */
    SDL_Color p2_body = {20, 55, 140, 255};
    draw_penguin(r->renderer, (int)g->p2.rect.x, (int)g->p2.rect.y, p2_body, 0);

    /* Bola */
    SDL_SetRenderDrawColor(r->renderer, 255, 255, 255, 255);
    SDL_Rect rb = { (int)g->ball.rect.x, (int)g->ball.rect.y,
                    (int)g->ball.rect.w, (int)g->ball.rect.h };
    SDL_RenderFillRect(r->renderer, &rb);

    /* Placar */
    char sc[32];
    snprintf(sc, sizeof(sc), "%d", g->p1.score);
    draw_text(r, r->font_lg, sc, SCREEN_W/2 - 80, 20, white);
    snprintf(sc, sizeof(sc), "%d", g->p2.score);
    draw_text(r, r->font_lg, sc, SCREEN_W/2 + 40, 20, white);

    /* Nomes */
    draw_text(r, r->font_sm, g->p1_name, PADDLE_MARGIN, SCREEN_H - 30, COLOR_ICE);
    int tw = 0;
    TTF_SizeUTF8(r->font_sm, g->p2_name, &tw, NULL);
    draw_text(r, r->font_sm, g->p2_name, SCREEN_W - PADDLE_MARGIN - tw, SCREEN_H - 30, COLOR_ICE);
}

static void render_paused(Renderer *r, const Game *g) {
    render_playing(r, g);
    draw_overlay(r, 140);
    draw_text_centered(r, r->font_lg, "PAUSADO", SCREEN_H/2 - 50, COLOR_WHITE);
    draw_text_centered(r, r->font_sm, "Pressione P para continuar", SCREEN_H/2 + 30, COLOR_ICE);
}

static void render_round_over(Renderer *r, const Game *g) {
    render_playing(r, g);
    draw_overlay(r, 100);
    SDL_Color white = COLOR_WHITE;
    char msg[64];
    snprintf(msg, sizeof(msg), "%d  x  %d", g->p1.score, g->p2.score);
    draw_text_centered(r, r->font_lg, msg, SCREEN_H/2 - 40, white);
}

static void render_game_over(Renderer *r, const Game *g) {
    draw_text_centered(r, r->font_lg, "FIM DE JOGO", 80, COLOR_WHITE);
    char msg[128];
    const char *winner = (g->p1.score > g->p2.score) ? g->p1_name : g->p2_name;
    snprintf(msg, sizeof(msg), "%s venceu!", winner);
    draw_text_centered(r, r->font_md, msg, 180, COLOR_GOLD);
    snprintf(msg, sizeof(msg), "%s  %d  x  %d  %s",
             g->p1_name, g->p1.score, g->p2.score, g->p2_name);
    draw_text_centered(r, r->font_md, msg, 240, COLOR_WHITE);
    draw_text_centered(r, r->font_sm, "R - Jogar novamente", 360, COLOR_ICE);
    draw_text_centered(r, r->font_sm, "M - Voltar ao menu",  400, COLOR_ICE);
    draw_text_centered(r, r->font_sm, "Resultado salvo no scoreboard!", 460, COLOR_ICE);
}

static void render_scoreboard(Renderer *r, const Scoreboard *sb) {
    draw_text_centered(r, r->font_md, "SCOREBOARD", 30, COLOR_WHITE);
    if (sb->count == 0) {
        draw_text_centered(r, r->font_sm, "Nenhuma partida registrada ainda.", 220, COLOR_ICE);
    } else {
        draw_text(r, r->font_sm, "#",          50,  90, COLOR_ICE);
        draw_text(r, r->font_sm, "Vencedor",   80,  90, COLOR_ICE);
        draw_text(r, r->font_sm, "Adversario", 280, 90, COLOR_ICE);
        draw_text(r, r->font_sm, "Placar",     460, 90, COLOR_ICE);
        draw_text(r, r->font_sm, "Data",       570, 90, COLOR_ICE);
        set_gray(r->renderer);
        SDL_RenderDrawLine(r->renderer, 40, 112, SCREEN_W - 40, 112);
        for (int i = 0; i < sb->count && i < 10; i++) {
            const ScoreEntry *e = &sb->entries[i];
            int y = 124 + i * 40;
            SDL_Color nc = (i == 0) ? COLOR_GOLD : COLOR_WHITE;
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            draw_text(r, r->font_sm, num, 50, y, nc);
            draw_text(r, r->font_sm, e->winner, 80, y, nc);
            const char *loser = (strcmp(e->winner, e->p1_name)==0) ? e->p2_name : e->p1_name;
            draw_text(r, r->font_sm, loser, 280, y, COLOR_ICE);
            char sc[16];
            snprintf(sc, sizeof(sc), "%d x %d", e->score_p1, e->score_p2);
            draw_text(r, r->font_sm, sc, 460, y, COLOR_WHITE);
            draw_text(r, r->font_sm, e->date, 570, y, COLOR_ICE);
        }
    }
    draw_text_centered(r, r->font_sm, "Pressione Esc ou M para voltar", 555, COLOR_ICE);
}

/* -----------------------------------------------------------------------
 * Frame principal
 * --------------------------------------------------------------------- */
void render_frame(Renderer *r, const Game *g, const Scoreboard *sb) {
    update_renderer_scale(r);
    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 255);
    SDL_RenderClear(r->renderer);
    draw_pixel_background(r->renderer);

    switch (g->state) {
        case STATE_MENU:        render_menu(r, g);        break;
        case STATE_OPTIONS:     render_options_menu(r, g); break;
        case STATE_NAME_INPUT:  render_name_input(r, g);  break;
        case STATE_PLAYING:     render_playing(r, g);     break;
        case STATE_PAUSED:      render_paused(r, g);      break;
        case STATE_ROUND_OVER:  render_round_over(r, g);  break;
        case STATE_GAME_OVER:   render_game_over(r, g);   break;
        case STATE_SCOREBOARD:  render_scoreboard(r, sb); break;
    }

    draw_custom_cursor(r);
    SDL_RenderPresent(r->renderer);
}
