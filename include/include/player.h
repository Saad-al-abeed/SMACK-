#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum
{
    PLAYER_IDLE,
    PLAYER_WALKING,
    PLAYER_JUMPING,
    PLAYER_ATTACKING,
    PLAYER_BLOCKING,
    PLAYER_DEATH,
    PLAYER_HURT
} PlayerState;

typedef enum
{
    FACING_RIGHT,
    FACING_LEFT
} PlayerDirection;

typedef struct
{
    // Position and physics
    float x, y;
    float velocity_x, velocity_y;
    float speed;
    float jump_force;
    float gravity;
    int current_attack;
    bool on_ground;

    // Animation
    SDL_Texture *idle_texture;
    SDL_Texture *walking_texture;
    SDL_Texture *jumping_texture;
    SDL_Texture *attack_texture;
    SDL_Texture *attack2_texture;
    SDL_Texture *attack3_texture;
    SDL_Texture *block_texture;
    SDL_Texture *death_texture;
    SDL_Texture *hurt_texture;

    SDL_Rect src_rect;
    SDL_Rect dest_rect;

    // Animation frames
    int current_frame;
    int frame_count;
    float frame_width;
    int frame_height;
    Uint32 last_frame_time;
    Uint32 frame_delay;

    // State
    PlayerState state;
    PlayerDirection direction;

    // Combat
    bool is_attacking;
    bool is_blocking;
    Uint32 attack_start_time;
    Uint32 attack_duration;
} Player;

// Function declarations
Player *create_player(SDL_Renderer *renderer, float x, float y);

void destroy_player(Player *player);
void handle_player_input(Player *player, const Uint8 *keystate);
void update_player(Player *player, Uint32 delta_time);
void render_player(SDL_Renderer *renderer, Player *player);
void set_player_state(Player *player, PlayerState new_state);

#endif // PLAYER_H