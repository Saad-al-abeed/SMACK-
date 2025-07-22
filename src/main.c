#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include "background.h"
#include "player.h"
#include "player2.h"

int main(int argc, char *argv[])
{
    // Error handlers for SDL subsystems initializations
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

    // Create window
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

    // Create renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

    // Create backgrounds
    Background *bg = create_background(ren, "assets/textures/intro_screen.bmp", 12, 100);
    if (!bg)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    Background *map1 = create_background(ren, "assets/textures/autumn.bmp", 12, 100);
    if (!map1)
    {
        destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    Background *map2 = create_background(ren, "assets/textures/cherry_blossom.bmp", 7, 100);
    if (!map2)
    {
        destroy_background(map1);
        destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    Background *map3 = create_background(ren, "assets/textures/sunset.bmp", 5, 100);
    if (!map3)
    {
        destroy_background(map2);
        destroy_background(map1);
        destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create buttons
    Button *play = create_button(ren, 560, 332,
                                 "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!play)
    {
        destroy_background(map3);
        destroy_background(map2);
        destroy_background(map1);
        destroy_background(bg);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *single_play = create_button(ren, 280, 400,
                                        "assets/textures/singleplayer_unselected.bmp", "assets/textures/singleplayer_selected.bmp");
    Button *multi_play = create_button(ren, 840, 400,
                                       "assets/textures/multiplayer_us.bmp", "assets/textures/multiplayer_s.bmp");
    Button *map1_btn = create_button(ren, 280, 400,
                                     "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    Button *map2_btn = create_button(ren, 560, 400,
                                     "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    Button *map3_btn = create_button(ren, 840, 400,
                                     "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");

    // Game state variables
    Player *player = NULL;
    Player2 *player2 = NULL; // For multiplayer
    Background *current_background = bg;
    bool game_started = false;
    Uint32 last_time = SDL_GetTicks();
    Uint32 delta_time;

    // UI state
    SDL_Event e;
    int running = 1;
    bool play_button_visible = true;
    bool single_play_button_visible = false;
    bool multi_play_button_visible = false;
    bool map1_btn_visible = false;
    bool map2_btn_visible = false;
    bool map3_btn_visible = false;

    // Main loop
    while (running)
    {
        // Calculate delta time
        Uint32 current_time = SDL_GetTicks();
        delta_time = current_time - last_time;
        last_time = current_time;

        // Handle events
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }

            // Only handle button events if button is visible
            if (play_button_visible)
            {
                handle_button_event(play, &e);
            }
            if (single_play_button_visible)
            {
                handle_button_event(single_play, &e);
            }
            if (multi_play_button_visible)
            {
                handle_button_event(multi_play, &e);
            }
            if (map1_btn_visible)
            {
                handle_button_event(map1_btn, &e);
            }
            if (map2_btn_visible)
            {
                handle_button_event(map2_btn, &e);
            }
            if (map3_btn_visible)
            {
                handle_button_event(map3_btn, &e);
            }
        }

        // Get keyboard state for player input
        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        const Uint8 *keystate2 = SDL_GetKeyboardState(NULL);

        // Handle player input if player exists and game has started
        if (player && game_started)
        {
            handle_player_input(player, keystate);
        }
        if (player2 && game_started)
        {
            handle_player2_input(player2, keystate2);
        }

        // Update current background
        update_background(current_background);

        // Update player if exists and game has started
        if (player && game_started)
        {
            update_player(player, delta_time);
        }
        if (player2 && game_started)
        {
            update_player2(player2, delta_time);
        }

        // Clear renderer
        SDL_RenderClear(ren);

        // Render current background
        render_background(ren, current_background);

        // Handle menu navigation
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
            map1_btn_visible = true;
            map2_btn_visible = true;
            map3_btn_visible = true;
        }

        if (multi_play_button_visible && multi_play_button_clicked(multi_play))
        {
            single_play_button_visible = false;
            multi_play_button_visible = false;
            map1_btn_visible = true;
            map2_btn_visible = true;
            map3_btn_visible = true;
        }

        // Handle map selection
        if (map1_btn_visible && map1_button_clicked(map1_btn))
        {
            map1_btn_visible = false;
            map2_btn_visible = false;
            map3_btn_visible = false;
            current_background = map1;
            game_started = true;

            // Create player when map is selected
            if (!player)
            {
                player = create_player(ren, 100, 375);
            }
            if (multi_play_button_clicked(multi_play))
            {
                player2 = create_player2(ren, 1000, 375);
            }
        }

        if (map2_btn_visible && map2_button_clicked(map2_btn))
        {
            map1_btn_visible = false;
            map2_btn_visible = false;
            map3_btn_visible = false;
            current_background = map2;
            game_started = true;

            // Create player when map is selected
            if (!player)
            {
                player = create_player(ren, 100, 375);
            }
            if (multi_play_button_clicked(multi_play))
            {
                player2 = create_player2(ren, 1000, 375);
            }
        }

        if (map3_btn_visible && map3_button_clicked(map3_btn))
        {
            map1_btn_visible = false;
            map2_btn_visible = false;
            map3_btn_visible = false;
            current_background = map3;
            game_started = true;

            // Create player when map is selected
            if (!player)
            {
                player = create_player(ren, 100, 375);
            }
            if (multi_play_button_clicked(multi_play))
            {
                player2 = create_player2(ren, 1000, 375);
            }
        }

        // Render UI elements
        if (play_button_visible)
        {
            render_button(ren, play);
        }
        if (single_play_button_visible)
        {
            render_button(ren, single_play);
        }
        if (multi_play_button_visible)
        {
            render_button(ren, multi_play);
        }
        if (map1_btn_visible)
        {
            render_button(ren, map1_btn);
        }
        if (map2_btn_visible)
        {
            render_button(ren, map2_btn);
        }
        if (map3_btn_visible)
        {
            render_button(ren, map3_btn);
        }

        // Render player if game has started
        if (player && game_started)
        {
            render_player(ren, player);
        }
        if (player2 && game_started)
        {
            render_player2(ren, player2);
        }

        // Present the rendered frame
        SDL_RenderPresent(ren);
    }

    // Cleanup
    if (player)
    {
        destroy_player(player);
    }
    if (player2)
    {
        destroy_player2(player2);
    }
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