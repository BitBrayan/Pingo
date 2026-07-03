#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "render.h"
#include "input.h"
#include "scoreboard.h"
#include "audio.h"

int main(void) {
    srand((unsigned int)time(NULL));

    /* Inicializa SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init erro: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init erro: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    /* Cria janela */
    SDL_Window *window = SDL_CreateWindow(
        "PONG",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
    );
    if (window) SDL_ShowCursor(SDL_DISABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow erro: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Cria renderer com VSync */
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!sdl_renderer) {
        fprintf(stderr, "SDL_CreateRenderer erro: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Inicializa renderer de jogo */
    Renderer renderer;
    if (render_init(&renderer, sdl_renderer) != 0) {
        fprintf(stderr, "Falha ao inicializar renderer\n");
        SDL_DestroyRenderer(sdl_renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Carrega scoreboard */
    Scoreboard sb;
    if (scoreboard_load(&sb, SCOREBOARD_FILE) == 0) {
        printf("Scoreboard carregado: %d entradas.\n", sb.count);
    } else {
        printf("Scoreboard novo criado.\n");
    }

    /* Estado do jogo */
    Game game;
    game_init(&game);

    /* Inicializa áudio (opcional) */
    if (audio_init() == 0) {
        audio_set_music_volume(game.music_volume);
        audio_set_sfx_volume(game.sfx_volume);
        /* tenta tocar a música de fundo se ela estiver presente */
        audio_play_music("sounds/theme.wav");
    }

    /* Loop principal */
    uint32_t prev_ticks = SDL_GetTicks();
    int running = 1;

    while (running) {
        /* Delta time */
        uint32_t now   = SDL_GetTicks();
        float    dt    = (now - prev_ticks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; /* cap em 50ms para evitar tunneling */
        prev_ticks = now;

        /* Processa eventos */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (input_handle(&event, &game, &sb, window)) {
                running = 0;
            }
        }

        /* Atualiza input contínuo */
        input_update_paddles(&game);

        /* Atualiza lógica */
        game_update(&game, dt);

        /* Renderiza */
        render_frame(&renderer, &game, &sb);
    }

    /* Limpeza */
    audio_destroy();
    render_destroy(&renderer);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    printf("Jogo encerrado.\n");
    return 0;
}
