#include "game_text.h"
#include <stdio.h>

// A global font that our functions will use
static TTF_Font* gFont = NULL;

// Helper function to render a single line of text
static void render_text(SDL_Renderer *renderer, const char *text, int x, int y, SDL_Color color) {
    if (!gFont) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, text, color);
    if (!textSurface) {
        fprintf(stderr, "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        fprintf(stderr, "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
    } else {
        SDL_Rect renderQuad = { x - textSurface->w / 2, y - textSurface->h / 2, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
        SDL_DestroyTexture(textTexture);
    }

    SDL_FreeSurface(textSurface);
}

bool text_init(const char* font_path, int font_size) {
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    gFont = TTF_OpenFont(font_path, font_size);
    if (!gFont) {
        fprintf(stderr, "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }
    
    return true;
}

void text_quit(void) {
    if (gFont) {
        TTF_CloseFont(gFont);
        gFont = NULL;
    }
    TTF_Quit();
}

void render_game_over_screen_single(SDL_Renderer *renderer, int winner) {
    const char* win_text = "";
    if (winner == 1) { // Player wins
        win_text = "You Win";
    } else { // Enemy wins
        win_text = "You got SMACKED!";
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {180, 180, 180, 255};

    // Render the main text in the middle of a 1280x720 screen
    render_text(renderer, win_text, 1280 / 2, 720 / 2 - 50, white);
    // Render the restart text below it
    render_text(renderer, "Press Enter to Restart The Match", 1280 / 2, 720 / 2 + 50, gray);
}

void render_game_over_screen_multi(SDL_Renderer *renderer, int winner) {
    char win_text[32];
    if (winner == 1) {
        sprintf(win_text, "Player 1 Wins");
    } else {
        sprintf(win_text, "Player 2 Wins");
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {180, 180, 180, 255};
    
    render_text(renderer, win_text, 1280 / 2, 720 / 2 - 50, white);
    render_text(renderer, "Press Enter to Restart The Match", 1280 / 2, 720 / 2 + 50, gray);
}