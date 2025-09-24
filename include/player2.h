#ifndef PLAYER2_H
#define PLAYER2_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef enum
{
    RIGHT = 1,
    LEFT = -1
} Player2Direction;

typedef enum
{
    PLAYER2_IDLE,
    PLAYER2_WALKING,
    PLAYER2_JUMPING,
    PLAYER2_ATTACKING,
    PLAYER2_BLOCKING,
    PLAYER2_DEATH,
    PLAYER2_HURT,
    /* NEW */
    PLAYER2_SLIDE,
    PLAYER2_BLOCK_HURT,
    PLAYER2_PRAY,
    PLAYER2_DOWN_ATTACK
} Player2State;

typedef struct
{
    /* Kinematics */
    float x, y;
    float velocity_x, velocity_y;
    float speed;
    float jump_force;
    float gravity;
    int current_attack;
    bool on_ground;

    /* Textures */
    SDL_Texture *idle_texture;
    SDL_Texture *walking_texture;
    SDL_Texture *jumping_texture;
    SDL_Texture *attack_texture;
    SDL_Texture *attack2_texture;
    SDL_Texture *attack3_texture;
    SDL_Texture *block_texture;
    SDL_Texture *death_texture;
    SDL_Texture *hurt_texture;

    /* NEW textures */
    SDL_Texture *slide_texture;
    SDL_Texture *block_hurt_texture;
    SDL_Texture *pray_texture;
    SDL_Texture *down_attack_texture;

    SDL_Rect src_rect;
    SDL_Rect dest_rect;

    int current_frame;
    int frame_count;
    float frame_width;
    int frame_height;
    Uint32 last_frame_time;
    Uint32 frame_delay;

    Player2State state;
    Player2Direction direction;

    /* Combat */
    bool is_attacking;
    bool is_blocking;
    Uint32 attack_start_time;
    Uint32 attack_duration;

    /* NEW: block-hurt timing */
    Uint32 block_hurt_start_time;
    Uint32 block_hurt_duration;
} Player2;

/* API */
Player2 *create_player2(SDL_Renderer *renderer, float x, float y);
void destroy_player2(Player2 *player);
void handle_player2_input(Player2 *player, const Uint8 *keystate);
void update_player2(Player2 *player, Uint32 delta_time);
void render_player2(SDL_Renderer *renderer, Player2 *player);
void set_player2_state(Player2 *player, Player2State new_state);

#endif /* PLAYER2_H */
