#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include "background.h"
// #include "player.h"

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

    // Create background
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
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create button
    Button *play = create_button(ren, 560, 332,
                                 "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!play)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *single_play = create_button(ren, 280, 400,
                                 "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!single_play)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *multi_play = create_button(ren, 840, 400,
                                 "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!multi_play)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *map1_btn = create_button(ren, 280, 400,
                                       "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!map1_btn)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *map2_btn = create_button(ren, 560, 400,
                                     "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!map2_btn)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Button *map3_btn = create_button(ren, 840, 400,
                                     "assets/textures/unselected-export.bmp", "assets/textures/selected-export.bmp");
    if (!map3_btn)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // For frame timing
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

        // Update game state
        update_background(bg);

        // Render
        SDL_RenderClear(ren);
        render_background(ren, bg);

        // Check if button was clicked and handle accordingly
        if (play_button_visible && play_button_clicked(play))
        {
            play_button_visible = false;   // Hide the button instead of destroying it
            //play->is_pressed = false; // Reset the pressed state
            single_play_button_visible = true;
            multi_play_button_visible = true;
            // You can add additional logic here when button is clicked
            // For example: start the game, change state, etc.
        }
        if (single_play_button_visible && single_play_button_clicked(single_play))
        {
            single_play_button_visible = false; // Hide the button instead of destroying it
            multi_play_button_visible = false;
            //single_play->is_pressed = false;    // Reset the pressed state
            // You can add additional logic here when button is clicked
            // For example: start the game, change state, etc.
        }
        if (multi_play_button_visible && multi_play_button_clicked(multi_play))
        {
            single_play_button_visible = false; // Hide the button instead of destroying it
            multi_play_button_visible = false;
            //multi_play->is_pressed = false;    // Reset the pressed state
            // You can add additional logic here when button is clicked
            // For example: start the game, change state, etc.
        }
        if (single_play_button_clicked(single_play) || multi_play_button_clicked(multi_play))
        {
            map1_btn_visible = true;
            map2_btn_visible = true;
            map3_btn_visible = true;
            if (map1_button_clicked(map1_btn))
            {
                map1_btn_visible = false;
                map2_btn_visible = false;
                map3_btn_visible = false;
                render_background(ren, map1);
                update_background(map1);
            }
            if (map2_button_clicked(map2_btn))
            {
                map1_btn_visible = false;
                map2_btn_visible = false;
                map3_btn_visible = false;
                render_background(ren, map2);
                update_background(map2);
            }
            if (map3_button_clicked(map3_btn))
            {
                map1_btn_visible = false;
                map2_btn_visible = false;
                map3_btn_visible = false;
                render_background(ren, map3);
                update_background(map3);
            }
        }

        // Only render button if it's visible
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

        SDL_RenderPresent(ren);
    }

    // Cleanup
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}