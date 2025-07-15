#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct
{
    SDL_Texture *texture;
    SDL_Rect src_rect;
    SDL_Rect dest_rect;
    int frame;
    int frame_count;
    Uint32 last_frame_time;
    Uint32 frame_delay;
    int facing_right;
    float velocity_x;
} Player;

void init_player(Player *player, SDL_Renderer *ren, const char *path,
                 int frame_count, int frame_delay, int x, int y);
void update_player(Player *player, Uint32 delta_time);
void render_player(SDL_Renderer *ren, Player *player);
void destroy_player(Player *player);

#endif