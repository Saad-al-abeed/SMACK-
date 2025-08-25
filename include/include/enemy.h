#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h" // Important for the AI to know about the player's state

// Represents the physical/animation state of the enemy
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

// Represents the AI's "thought process" or intent
typedef enum {
    AI_DECIDING,      // The AI is thinking what to do next
    AI_ENGAGING,      // The AI is actively moving towards the player
    AI_REPOSITIONING, // The AI is moving to a better position (e.g., backing away)
    AI_ATTACKING,     // The AI is executing an attack
    AI_BLOCKING       // The AI is holding a defensive stance
} AIState;

// Represents the direction the enemy sprite is facing
typedef enum
{
    R, // Right
    L  // Left
} EnemyDirection;

typedef struct
{
    // -- AI Fields --
    AIState ai_state;         // The current "thought" of the AI
    Uint32 ai_action_timer;   // A timer to prevent the AI from changing its mind too quickly

    // -- Physics & Position --
    float x, y;
    float velocity_x, velocity_y;
    float speed;
    float jump_force;
    float gravity;
    bool on_ground;

    // -- Animation --
    SDL_Texture *idle_texture;
    SDL_Texture *walking_texture;
    SDL_Texture *jumping_texture;
    SDL_Texture *attack_texture;
    SDL_Texture *attack2_texture;
    SDL_Texture *attack3_texture;
    SDL_Texture *block_texture;
    SDL_Texture *hurt_texture;
    SDL_Texture *death_texture;

    SDL_Rect src_rect;  // The part of the texture to draw
    SDL_Rect dest_rect; // Where on the screen to draw it

    int current_frame;
    int frame_count;
    float frame_width;
    int frame_height;
    Uint32 last_frame_time;
    Uint32 frame_delay;

    // -- State & Combat --
    EnemyState state;         // The current animation/physical state
    EnemyDirection direction;
    int current_attack;       // Which attack in the combo (0, 1, or 2)
    bool is_attacking;
    bool is_blocking;
    Uint32 attack_start_time;
    Uint32 attack_duration;

} Enemy;

// Function declarations
Enemy *create_enemy(SDL_Renderer *renderer, float x, float y);
void destroy_enemy(Enemy *enemy);

// The completely new AI logic
void handle_enemy_ai(Enemy *enemy, Player *player, float delta_time);

void update_enemy(Enemy *enemy, Uint32 delta_time);
void render_enemy(SDL_Renderer *renderer, Enemy *enemy);
void set_enemy_state(Enemy *enemy, EnemyState new_state);

#endif // ENEMY_H