#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h"

typedef enum
{
    ENEMY_IDLE,
    ENEMY_WALKING,
    ENEMY_JUMPING,
    ENEMY_ATTACKING,
    ENEMY_BLOCKING,
    ENEMY_DEATH,
    ENEMY_HURT
} EnemyState;

typedef enum
{
    R,
    L
} EnemyDirection;

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
    SDL_Texture *hurt_texture;
    SDL_Texture *death_texture;

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
    EnemyState state;
    EnemyDirection direction;

    // Combat
    bool is_attacking;
    bool is_blocking;
    Uint32 attack_start_time;
    Uint32 attack_duration;
} Enemy;

// Function declarations
Enemy *create_enemy(SDL_Renderer *renderer, float x, float y);

void destroy_enemy(Enemy *enemy);
void handle_enemy_ai(Enemy *enemy, Player *player, float delta_time);
void update_enemy(Enemy *enemy, Uint32 delta_time);
void render_enemy(SDL_Renderer *renderer, Enemy *enemy);
void set_enemy_state(Enemy *enemy, EnemyState new_state);

#endif // ENEMY_H