#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"

void game_init(Game *g) {
    memset(g, 0, sizeof(Game));
    g->state          = STATE_MENU;
    g->menu_sel       = 0;
    g->options_sel    = 0;
    g->video_mode     = VIDEO_FULLSCREEN;
    g->resolution_idx = 3;
    g->music_volume   = 40;
    g->sfx_volume     = 40;
    strcpy(g->p1_name, "Jogador 1");
    strcpy(g->p2_name, "Jogador 2");
}

void game_reset_ball(Game *g, int dir) {
    g->ball.rect.x = (SCREEN_W - BALL_SIZE) / 2.0f;
    g->ball.rect.y = (SCREEN_H - BALL_SIZE) / 2.0f;
    g->ball.rect.w = BALL_SIZE;
    g->ball.rect.h = BALL_SIZE;
    g->ball.speed  = BALL_SPEED_INIT;

    float angle = ((float)(rand() % 61) - 30.0f) * (3.14159f / 180.0f);
    g->ball.vx = dir * g->ball.speed * cosf(angle);
    g->ball.vy = g->ball.speed * sinf(angle);
}

void game_reset_round(Game *g) {
    g->p1.rect.x = PADDLE_MARGIN;
    g->p1.rect.y = (SCREEN_H - PADDLE_H) / 2.0f;
    g->p1.rect.w = PADDLE_W;
    g->p1.rect.h = PADDLE_H;
    g->p1.vy     = 0;

    g->p2.rect.x = SCREEN_W - PADDLE_MARGIN - PADDLE_W;
    g->p2.rect.y = (SCREEN_H - PADDLE_H) / 2.0f;
    g->p2.rect.w = PADDLE_W;
    g->p2.rect.h = PADDLE_H;
    g->p2.vy     = 0;
}

void game_start_match(Game *g) {
    g->p1.score = 0;
    g->p2.score = 0;
    game_reset_round(g);
    game_reset_ball(g, (rand() % 2) ? 1 : -1);
    g->state = STATE_PLAYING;
}

static int rect_overlap(const Rect *a, const Rect *b) {
    return (a->x < b->x + b->w &&
            a->x + a->w > b->x &&
            a->y < b->y + b->h &&
            a->y + a->h > b->y);
}

void game_update(Game *g, float dt) {
    if (g->state == STATE_ROUND_OVER) {
        g->round_timer -= (int)(dt * 1000.0f);
        if (g->round_timer <= 0) {
            game_reset_round(g);
            game_reset_ball(g, (rand() % 2) ? 1 : -1);
            g->state = STATE_PLAYING;
        }
        return;
    }

    if (g->state != STATE_PLAYING) return;

    g->p1.rect.y += g->p1.vy * dt;
    g->p2.rect.y += g->p2.vy * dt;

    if (g->p1.rect.y < 0) g->p1.rect.y = 0;
    if (g->p1.rect.y + PADDLE_H > SCREEN_H) g->p1.rect.y = SCREEN_H - PADDLE_H;
    if (g->p2.rect.y < 0) g->p2.rect.y = 0;
    if (g->p2.rect.y + PADDLE_H > SCREEN_H) g->p2.rect.y = SCREEN_H - PADDLE_H;

    if (g->mode == MODE_1P) {
        float ball_cy = g->ball.rect.y + BALL_SIZE / 2.0f;
        float pad_cy  = g->p2.rect.y  + PADDLE_H / 2.0f;
        float diff = ball_cy - pad_cy;
        if (fabsf(diff) > 4.0f) {
            g->p2.vy = (diff > 0 ? 1.0f : -1.0f) * CPU_SPEED;
        } else {
            g->p2.vy = 0;
        }
    }

    g->ball.rect.x += g->ball.vx * dt;
    g->ball.rect.y += g->ball.vy * dt;

    if (g->ball.rect.y <= 0) {
        g->ball.rect.y = 0;
        g->ball.vy = fabsf(g->ball.vy);
    }
    if (g->ball.rect.y + BALL_SIZE >= SCREEN_H) {
        g->ball.rect.y = SCREEN_H - BALL_SIZE;
        g->ball.vy = -fabsf(g->ball.vy);
    }

    if (g->ball.vx < 0 && rect_overlap(&g->ball.rect, &g->p1.rect)) {
        float rel = ((g->ball.rect.y + BALL_SIZE / 2.0f) - (g->p1.rect.y + PADDLE_H / 2.0f))
                    / (PADDLE_H / 2.0f);
        if (rel < -1.0f) rel = -1.0f;
        if (rel >  1.0f) rel =  1.0f;

        float angle = rel * (60.0f * 3.14159f / 180.0f);
        g->ball.speed *= BALL_SPEED_INC;
        if (g->ball.speed > BALL_SPEED_MAX) g->ball.speed = BALL_SPEED_MAX;

        g->ball.vx = g->ball.speed * cosf(angle);
        g->ball.vy = g->ball.speed * sinf(angle);
        g->ball.rect.x = g->p1.rect.x + PADDLE_W + 1.0f;
        /* Som de batida */
        audio_play_sfx(SFX_HIT);
    }

    if (g->ball.vx > 0 && rect_overlap(&g->ball.rect, &g->p2.rect)) {
        float rel = ((g->ball.rect.y + BALL_SIZE / 2.0f) - (g->p2.rect.y + PADDLE_H / 2.0f))
                    / (PADDLE_H / 2.0f);
        if (rel < -1.0f) rel = -1.0f;
        if (rel >  1.0f) rel =  1.0f;

        float angle = rel * (60.0f * 3.14159f / 180.0f);
        g->ball.speed *= BALL_SPEED_INC;
        if (g->ball.speed > BALL_SPEED_MAX) g->ball.speed = BALL_SPEED_MAX;

        g->ball.vx = -g->ball.speed * cosf(angle);
        g->ball.vy =  g->ball.speed * sinf(angle);
        g->ball.rect.x = g->p2.rect.x - BALL_SIZE - 1.0f;
        /* Som de batida */
        audio_play_sfx(SFX_HIT);
    }

    if (g->ball.rect.x + BALL_SIZE < 0) {
        g->p2.score++;
        audio_play_sfx(SFX_SCORE);
        if (g->p2.score >= WIN_SCORE) {
            g->state = STATE_GAME_OVER;
        } else {
            g->round_timer = 2000;
            g->state = STATE_ROUND_OVER;
        }
    } else if (g->ball.rect.x > SCREEN_W) {
        g->p1.score++;
        audio_play_sfx(SFX_SCORE);
        if (g->p1.score >= WIN_SCORE) {
            g->state = STATE_GAME_OVER;
        } else {
            g->round_timer = 2000;
            g->state = STATE_ROUND_OVER;
        }
    }
}
