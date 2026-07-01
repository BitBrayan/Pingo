#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define SCREEN_W 800
#define SCREEN_H 600

#define PADDLE_W    14
#define PADDLE_H    80
#define BALL_SIZE    8
#define PADDLE_MARGIN 20

#define PADDLE_SPEED    400.0f
#define BALL_SPEED_INIT 300.0f
#define BALL_SPEED_INC  1.05f
#define BALL_SPEED_MAX  700.0f
#define CPU_SPEED       320.0f

#define WIN_SCORE 7

#define MODE_1P  0
#define MODE_2P  1

typedef enum {
    STATE_MENU,
    STATE_NAME_INPUT,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_ROUND_OVER,
    STATE_GAME_OVER,
    STATE_SCOREBOARD
} GameState;

typedef struct {
    float x, y, w, h;
} Rect;

typedef struct {
    Rect  rect;
    float vy;     
    int   score;
} Paddle;


typedef struct {
    Rect  rect;
    float vx, vy;
    float speed;
} Ball;

typedef struct {
    GameState state;
    int       mode;        
    Paddle    p1, p2;
    Ball      ball;
    int       round_timer;   

    char p1_name[32];
    char p2_name[32];
    int  name_input_field; 

    int  menu_sel;         
    char input_buf[32];
    int  input_len;
} Game;

void game_init(Game *g);
void game_reset_ball(Game *g, int dir);
void game_reset_round(Game *g);
void game_update(Game *g, float dt);
void game_start_match(Game *g);

#endif
