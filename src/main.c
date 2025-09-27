#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "sound.h"
#include "background.h"
#include "player.h"
#include "player2.h"
#include "multifight.h"
#include "singlefight.h"
#include "game_text.h" // ADDED: Include for text rendering

/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    srand(time(NULL));
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
    
    // MODIFIED: Replaced TTF_Init() with our new text_init() function
    if (!text_init("assets/texts/Pixelify_Sans/static/PixelifySans-Medium.ttf", 48)) {
        fprintf(stderr, "Failed to initialize text module.\n");
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    sound_init();

    /* ---------- window / renderer ---------- */
    SDL_Window *win = SDL_CreateWindow(
        "SMACK!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN);
    if (!win)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        text_quit(); // MODIFIED: Cleanup text module
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
        text_quit(); // MODIFIED: Cleanup text module
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
                                     "assets/textures/map1us-export.bmp",
                                     "assets/textures/map1s-export.bmp");
    Button *map2_btn = create_button(ren, 560, 400,
                                     "assets/textures/map2us-export.bmp",
                                     "assets/textures/map2s-export.bmp");
    Button *map3_btn = create_button(ren, 840, 400,
                                     "assets/textures/map3us-export.bmp",
                                     "assets/textures/map3s-export.bmp");

    /* ---------- game-state variables ---------- */
    Player *player = NULL;
    Player2 *player2 = NULL;  
    MultiFight *mulfight = NULL; 
    SingleFight *sinfight = NULL;
    Enemy *enemy = NULL;

    Background *current_background = bg;
    bool game_started = false;
    bool is_multiplayer = false;

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
    sound_play_music("menu");
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

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);

        // ADDED: Main game logic is now split between "fight ongoing" and "fight over"
        bool fight_is_over = (is_multiplayer && mulfight && mulfight->fight_over) || (!is_multiplayer && sinfight && sinfight->fight_over);

        if (game_started && !fight_is_over)
        {
            // --- LOGIC FOR WHEN FIGHT IS ONGOING ---
            if (player) handle_player_input(player, keystate);
            if (player2) handle_player2_input(player2, keystate);
            if (enemy) handle_enemy_ai(enemy, player, delta_time);

            if (player) update_player(player, delta_time);
            if (player2) update_player2(player2, delta_time);
            if (enemy) update_enemy(enemy, delta_time);

            if (mulfight) update_multi_fight(mulfight, player, player2, delta_time);
            if (sinfight) update_single_fight(sinfight, player, enemy, delta_time);
        }
        else if (game_started && fight_is_over)
        { 
            if (player) update_player(player, delta_time);
            if (player2) update_player2(player2, delta_time);
            if (enemy) update_enemy(enemy, delta_time); 
            if (is_multiplayer && mulfight)
            {
                handle_multi_fight_game_over_input(mulfight, keystate);
                if (mulfight->restart_requested)
                {
                    destroy_player(player);
                    destroy_player2(player2);
                    destroy_multi_fight(mulfight);
                    player = create_player(ren, 50, 375);
                    player2 = create_player2(ren, 800, 375);
                    mulfight = create_multi_fight();
                }
            }
            else if (!is_multiplayer && sinfight)
            {
                handle_single_fight_game_over_input(sinfight, keystate);
                if (sinfight->restart_requested)
                {
                    destroy_player(player);
                    destroy_enemy(enemy);
                    destroy_single_fight(sinfight);
                    player = create_player(ren, 50, 375);
                    enemy = create_enemy(ren, 800, 375);
                    sinfight = create_single_fight();
                }
            }
        }

        /* background update (always runs) */
        update_background(current_background);
        
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
            is_multiplayer = false;
        }

        if (multi_play_button_visible && multi_play_button_clicked(multi_play))
        {
            single_play_button_visible = false;
            multi_play_button_visible = false;
            map1_btn_visible = map2_btn_visible = map3_btn_visible = true;
            is_multiplayer = true;
        }

        /* map selections now check for both modes */
        if (map1_btn_visible && map1_button_clicked(map1_btn))
        {
            sound_play_music("map1");
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map1; game_started = true;
            player = create_player(ren, 50, 375);
            if (is_multiplayer) {
                player2 = create_player2(ren, 800, 375);
                mulfight = create_multi_fight();
            } else {
                enemy = create_enemy(ren, 800, 375);
                sinfight = create_single_fight();
            }
        }
        if (map2_btn_visible && map2_button_clicked(map2_btn))
        {
            sound_play_music("map2");
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map2; game_started = true;
            player = create_player(ren, 50, 375);
            if (is_multiplayer) {
                player2 = create_player2(ren, 800, 375);
                mulfight = create_multi_fight();
            } else {
                enemy = create_enemy(ren, 800, 375);
                sinfight = create_single_fight();
            }
        }
        if (map3_btn_visible && map3_button_clicked(map3_btn))
        {
            sound_play_music("map3");
            map1_btn_visible = map2_btn_visible = map3_btn_visible = false;
            current_background = map3; game_started = true;
            player = create_player(ren, 50, 375);
            if (is_multiplayer) {
                player2 = create_player2(ren, 800, 375);
                mulfight = create_multi_fight();
            } else {
                enemy = create_enemy(ren, 800, 375);
                sinfight = create_single_fight();
            }
        }

        /* ---------- rendering ---------- */
        SDL_RenderClear(ren);
        render_background(ren, current_background);

        if (game_started) {
            sound_play_effects(player, player2, enemy);
            if (player) render_player(ren, player);
            if (player2) render_player2(ren, player2);
            if (enemy) render_enemy(ren, enemy);
            if (mulfight) render_health_bars(ren, mulfight);
            if (sinfight) health_bars(ren, sinfight);

            // ADDED: Render the game over screen on top if the fight is over
            if (is_multiplayer && mulfight && mulfight->fight_over) {
                render_game_over_screen_multi(ren, mulfight->winner);
            } else if (!is_multiplayer && sinfight && sinfight->fight_over) {
                render_game_over_screen_single(ren, sinfight->winner);
            }
        } else {
             if (play_button_visible) render_button(ren, play);
             if (single_play_button_visible) render_button(ren, single_play);
             if (multi_play_button_visible) render_button(ren, multi_play);
             if (map1_btn_visible) render_button(ren, map1_btn);
             if (map2_btn_visible) render_button(ren, map2_btn);
             if (map3_btn_visible) render_button(ren, map3_btn);
        }

        SDL_RenderPresent(ren);
    }

    /* ---------- cleanup ---------- */
    if (player) destroy_player(player);
    if (player2) destroy_player2(player2);
    if (enemy) destroy_enemy(enemy); 
    if (sinfight) destroy_single_fight(sinfight);
    if (mulfight) destroy_multi_fight(mulfight);

    sound_quit();
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
    text_quit(); // ADDED: Cleanup for the text module
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}