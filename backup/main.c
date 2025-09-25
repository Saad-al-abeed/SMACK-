#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>

#include "background.h"
#include "player.h"
#include "player2.h"
#include "multifight.h"
#include "singlefight.h"

/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    /* ---------- SDL / libraries initialisation ---------- */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        fprintf(stderr, "Mix_OpenAudio Error: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    /* ---------- window / renderer ---------- */
    SDL_Window *win = SDL_CreateWindow(
        "SMACK!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN);
    if (!win)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
    {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    /* ---------- resources ---------- */
    Background *bg = create_background(ren, "assets/textures/intro_screen.bmp", 12, 100);
    Background *map1 = create_background(ren, "assets/textures/autumn.bmp", 12, 100);
    Background *map2 = create_background(ren, "assets/textures/cherry_blossom.bmp", 7, 100);
    Background *map3 = create_background(ren, "assets/textures/sunset.bmp", 5, 100);

    Button *play = create_button(ren, 560, 332,
                                 "assets/textures/unselected-export.bmp",
                                 "assets/textures/selected-export.bmp");

    Button *single_play = create_button(ren, 280, 400,
                                        "assets/textures/singleplayer_unselected.bmp",
                                        "assets/textures/singleplayer_selected.bmp");
    Button *multi_play = create_button(ren, 840, 400,
                                       "assets/textures/multiplayer_us.bmp",
                                       "assets/textures/multiplayer_s.bmp");

    Button *map1_btn = create_button(ren, 280, 400,
                                     "assets/textures/unselected-export.bmp",
                                     "assets/textures/selected-export.bmp");
    Button *map2_btn = create_button(ren, 560, 400,
                                     "assets/textures/unselected-export.bmp",
                                     "assets/textures/selected-export.bmp");
    Button *map3_btn = create_button(ren, 840, 400,
                                     "assets/textures/unselected-export.bmp",
                                     "assets/textures/selected-export.bmp");

    /* ---------- game-state variables ---------- */
    Player *player = NULL;
    Player2 *player2 = NULL;  
    MultiFight *mulfight = NULL; 
    SingleFight *sinfight = NULL;
    Enemy *enemy = NULL;

    Background *current_background = bg;
    bool game_started = false;
    bool is_multiplayer = false; /* NEW */

    Uint32 last_time = SDL_GetTicks();
    Uint32 delta_time;

    /* ---------- UI visibility flags ---------- */
    bool play_button_visible = true;
    bool single_play_button_visible = false;
    bool multi_play_button_visible = false;
    bool map1_btn_visible = false;
    bool map2_btn_visible = false;
    bool map3_btn_visible = false;

    /* ---------- main loop ---------- */
    SDL_Event e;
    int running = 1;

    while (running)
    {
        /* delta-time calculation */
        Uint32 current_time = SDL_GetTicks();
        delta_time = current_time - last_time;
        last_time = current_time;

        /* events */
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = 0;

            if (play_button_visible)
                handle_button_event(play, &e);
            if (single_play_button_visible)
                handle_button_event(single_play, &e);
            if (multi_play_button_visible)
                handle_button_event(multi_play, &e);
            if (map1_btn_visible)
                handle_button_event(map1_btn, &e);
            if (map2_btn_visible)
                handle_button_event(map2_btn, &e);
            if (map3_btn_visible)
                handle_button_event(map3_btn, &e);
        }

        /* keyboard state */
        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        if (player && game_started)
            handle_player_input(player, keystate);
        if (player2 && game_started)
            handle_player2_input(player2, keystate);
        if (!is_multiplayer && game_started && enemy)
            handle_enemy_ai(enemy, player, delta_time);

        /* background update */
        update_background(current_background);

        /* players update */
        if (player && game_started)
            update_player(player, delta_time);
        if (player2 && game_started)
            update_player2(player2, delta_time);
        if (enemy && game_started)
            update_enemy(enemy, delta_time); /* NEW */

        if (mulfight && game_started)
            update_multi_fight(mulfight, player, player2, delta_time);
        if (sinfight && !is_multiplayer && game_started)
            update_single_fight(sinfight, player, enemy, delta_time);

        /* ---------- menu navigation ---------- */
        if (play_button_visible && play_button_clicked(play))
        {
            play_button_visible = false;
            single_play_button_visible = true;
            multi_play_button_visible = true;
        }

        if (single_play_button_visible && single_play_button_clicked(single_play))
        {
            single_play_button_visible = false;
            multi_play_button_visible = false;
            map1_btn_visible = map2_btn_visible = map3_btn_visible = true;
            is_multiplayer = false; /* NEW */
        }

        if (multi_play_button_visible && multi_play_button_clicked(multi_play))
        {
            single_play_button_visible = false;
            multi_play_button_visible = false;
            map1_btn_visible = map2_btn_visible = map3_btn_visible = true;
            is_multiplayer = true; /* NEW */
        }

        /* ---------- map 1 selection ---------- */
        if (map1_btn_visible && map1_button_clicked(map1_btn))
        {
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map1;
            game_started = true;

            if (!player)
                player = create_player(ren, 100, 375);
            if (is_multiplayer && !player2)
                player2 = create_player2(ren, 800, 375);
            if (!is_multiplayer && !enemy)
                enemy = create_enemy(ren, 800, 375);          /* NEW */
            if (!mulfight && is_multiplayer && player && player2) /* NEW */
                mulfight = create_multi_fight();
            if (!sinfight && !is_multiplayer && player && enemy) /* NEW */
                sinfight = create_single_fight();
        }

        /* ---------- map 2 selection ---------- */
        if (map2_btn_visible && map2_button_clicked(map2_btn))
        {
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map2;
            game_started = true;

            if (!player)
                player = create_player(ren, 100, 375);
            if (is_multiplayer && !player2)
                player2 = create_player2(ren, 800, 375);
            if (!is_multiplayer && !enemy)
                enemy = create_enemy(ren, 800, 375);             /* NEW */
            if (!mulfight && is_multiplayer && player && player2) /* NEW */
                mulfight = create_multi_fight();
            if (!sinfight && !is_multiplayer && player && enemy) /* NEW */
                sinfight = create_single_fight();
        }

        /* ---------- map 3 selection ---------- */
        if (map3_btn_visible && map3_button_clicked(map3_btn))
        {
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map3;
            game_started = true;

            if (!player)
                player = create_player(ren, 100, 375);
            if (is_multiplayer && !player2)
                player2 = create_player2(ren, 800, 375);
            if (!is_multiplayer && !enemy)
                enemy = create_enemy(ren, 800, 375);             /* NEW */
            if (!mulfight && is_multiplayer && player && player2) /* NEW */
                mulfight = create_multi_fight();
            if (!sinfight && !is_multiplayer && player && enemy) /* NEW */
                sinfight = create_single_fight();
        }

        /* ---------- rendering ---------- */
        SDL_RenderClear(ren);

        render_background(ren, current_background);

        if (play_button_visible)
            render_button(ren, play);
        if (single_play_button_visible)
            render_button(ren, single_play);
        if (multi_play_button_visible)
            render_button(ren, multi_play);
        if (map1_btn_visible)
            render_button(ren, map1_btn);
        if (map2_btn_visible)
            render_button(ren, map2_btn);
        if (map3_btn_visible)
            render_button(ren, map3_btn);

        if (player && game_started)
            render_player(ren, player);
        if (player2 && game_started)
            render_player2(ren, player2);
        if (enemy && game_started)
            render_enemy(ren, enemy);
        if (mulfight && game_started)
            render_health_bars(ren, mulfight);
        if (sinfight && !is_multiplayer && game_started)
            health_bars(ren, sinfight);

        SDL_RenderPresent(ren);
    }

    /* ---------- cleanup ---------- */
    if (player)
        destroy_player(player);
    if (player2)
        destroy_player2(player2);
    if (enemy)
        destroy_enemy(enemy); 
    if (sinfight) 
        destroy_single_fight(sinfight);
    if (mulfight)
        destroy_multi_fight(mulfight);

    destroy_button(play);
    destroy_button(single_play);
    destroy_button(multi_play);
    destroy_button(map1_btn);
    destroy_button(map2_btn);
    destroy_button(map3_btn);

    destroy_background(bg);
    destroy_background(map1);
    destroy_background(map2);
    destroy_background(map3);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}