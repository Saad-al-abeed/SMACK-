#include "player.h"
#include <stdio.h>

void init_player(Player *player, SDL_Renderer *ren, const char *path,
                 int frame_count, int frame_delay, int x, int y)
{
    player->texture = IMG_LoadTexture(ren, path);
    if (!player->texture)
    {
        fprintf(stderr, "Failed to load player texture: %s\n", IMG_GetError());
        return;
    }

    int tex_w, tex_h;
    SDL_QueryTexture(player->texture, NULL, NULL, &tex_w, &tex_h);

    player->frame = 0;
    player->frame_count = frame_count;
    player->last_frame_time = SDL_GetTicks();
    player->frame_delay = frame_delay;
    player->facing_right = 1;

    player->src_rect.w = tex_w / frame_count;
    player->src_rect.h = tex_h;
    player->src_rect.x = 0;
    player->src_rect.y = 0;

    player->dest_rect.w = player->src_rect.w;
    player->dest_rect.h = player->src_rect.h;
    player->dest_rect.x = x;
    player->dest_rect.y = y;

    player->velocity_x = 0;
}

void update_player(Player *player, Uint32 delta_time)
{
    if (SDL_GetTicks() - player->last_frame_time > player->frame_delay)
    {
        player->frame = (player->frame + 1) % player->frame_count;
        player->src_rect.x = player->frame * player->src_rect.w;
        player->last_frame_time = SDL_GetTicks();
    }

    player->dest_rect.x += player->velocity_x * (delta_time / 10.0f);

    if (player->dest_rect.x < 0)
    {
        player->dest_rect.x = 0;
    }
    if (player->dest_rect.x > 1280 - player->dest_rect.w)
    {
        player->dest_rect.x = 1280 - player->dest_rect.w;
    }
}

void render_player(SDL_Renderer *ren, Player *player)
{
    SDL_RendererFlip flip = player->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(ren, player->texture, &player->src_rect, &player->dest_rect, 0, NULL, flip);
}

void destroy_player(Player *player)
{
    if (player->texture)
    {
        SDL_DestroyTexture(player->texture);
    }
}