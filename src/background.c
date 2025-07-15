#include "background.h"
#include <stdio.h>
#include <stdlib.h>

Background *create_background(SDL_Renderer *ren, const char *path)
{
    Background *bg = (Background *)malloc(sizeof(Background));
    if (!bg)
    {
        fprintf(stderr, "Failed to allocate background\n");
        return NULL;
    }

    bg->texture = IMG_LoadTexture(ren, path);
    if (!bg->texture)
    {
        fprintf(stderr, "IMG_LoadTexture Error: %s\n", IMG_GetError());
        free(bg);
        return NULL;
    }

    return bg;
}

void render_background(SDL_Renderer *ren, Background *bg)
{
    if (bg && bg->texture)
    {
        SDL_RenderCopy(ren, bg->texture, NULL, NULL);
    }
}

void destroy_background(Background *bg)
{
    if (bg)
    {
        if (bg->texture)
        {
            SDL_DestroyTexture(bg->texture);
        }
        free(bg);
    }
}