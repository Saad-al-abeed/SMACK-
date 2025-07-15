#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct
{
    SDL_Texture *texture;
} Background;

Background *create_background(SDL_Renderer *ren, const char *path);
void render_background(SDL_Renderer *ren, Background *bg);
void destroy_background(Background *bg);

#endif