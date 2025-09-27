#ifndef GAME_TEXT_H
#define GAME_TEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

bool text_init(const char* font_path, int font_size);

void text_quit(void);

void render_game_over_screen_single(SDL_Renderer *renderer, int winner);

void render_game_over_screen_multi(SDL_Renderer *renderer, int winner);

#endif // GAME_TEXT_H