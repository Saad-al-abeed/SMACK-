#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "background.h"
//#include "player.h"

int main(int argc, char *argv[])
{
    // Initialize SDL subsystems
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
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
    Background *bg = create_background(ren, "assets/textures/background1.bmp");
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

    // Initialize player
    // Player player;
    // init_player(&player, ren, "assets/textures/WALK-Sheet-export.bmp", 8, 100, 600, 400);

    // For frame timing
    Uint32 last_time = SDL_GetTicks();
    SDL_Event e;
    int running = 1;

    // Main loop
    while (running)
    {
        Uint32 current_time = SDL_GetTicks();
        Uint32 delta_time = current_time - last_time;
        last_time = current_time;

        // Handle events
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }

            // Handle key presses
            // if (e.type == SDL_KEYDOWN)
            // {
            //     switch (e.key.keysym.sym)
            //     {
            //     case SDLK_LEFT:
            //         player.velocity_x = -3.0f;
            //         player.facing_right = 0;
            //         break;
            //     case SDLK_RIGHT:
            //         player.velocity_x = 3.0f;
            //         player.facing_right = 1;
            //         break;
            //     case SDLK_ESCAPE:
            //         running = 0;
            //         break;
            //     }
            // }

            // Handle key releases
            // if (e.type == SDL_KEYUP)
            // {
            //     switch (e.key.keysym.sym)
            //     {
            //     case SDLK_LEFT:
            //         if (player.velocity_x < 0)
            //             player.velocity_x = 0;
            //         break;
            //     case SDLK_RIGHT:
            //         if (player.velocity_x > 0)
            //             player.velocity_x = 0;
            //         break;
            //     }
            // }
        }

        // Update game state
        //update_player(&player, delta_time);

        // Render
        SDL_RenderClear(ren);
        render_background(ren, bg);
        //render_player(ren, &player);
        SDL_RenderPresent(ren);
    }

    // Cleanup
    destroy_background(bg);
    //destroy_player(&player);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}