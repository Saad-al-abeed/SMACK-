#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include <player.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Match player's facing enum semantics */
    typedef enum
    {
        L = 0,
        R = 1
    } EnemyDirection;
    typedef enum {
        AI_DECIDING,      // The AI is thinking what to do next
        AI_ENGAGING,      // The AI is actively moving towards the player
        AI_REPOSITIONING, // The AI is moving to a better position (e.g., backing away)
        AI_ATTACKING,     // The AI is executing an attack
        AI_BLOCKING,     // The AI is holding a defensive stance
        AI_EVADING
    } AIState;
    /* Mirror all player states (incl. new ones) */
    typedef enum
    {
        ENEMY_IDLE = 0,
        ENEMY_WALKING,
        ENEMY_JUMPING,
        ENEMY_ATTACKING,
        ENEMY_BLOCKING,
        ENEMY_HURT,
        ENEMY_DEATH,
        ENEMY_SLIDE,
        ENEMY_BLOCK_HURT,
        ENEMY_PRAY,
        ENEMY_DOWN_ATTACK,
        ENEMY_REPOSITIONING 
    } EnemyState;

    typedef struct Enemy
    {
        // -- AI Fields --
        AIState ai_state;         // The current "thought" of the AI
        Uint32 ai_action_timer;   // A timer to prevent the AI from changing its mind too quickly
        Uint32 block_start_time;
        /* --- Transform / physics --- */
        float x, y;
        float velocity_x, velocity_y;
        float speed;
        float jump_force;
        float gravity;
        int on_ground; /* bool-like */

        /* --- Facing & state --- */
        EnemyDirection direction;
        EnemyState state;

        /* --- Combat flags & timers (mirror player) --- */
        int is_attacking; /* bool-like */
        int is_blocking;  /* bool-like */
        Uint32 attack_start_time;
        Uint32 attack_duration; /* how long a single attack anim lasts (ms) */
        int current_attack;     /* 0/1/2 combo step */

        /* --- Block-hurt timer (mirror player) --- */
        Uint32 block_hurt_start_time;
        Uint32 block_hurt_duration; /* ms */

        /* --- Health / lifecycle (kept here so single/multi fight can read) --- */
        int health;
        int is_dead; /* bool-like */
        Uint32 death_start_time;

        /* --- Textures (mirror player assets) --- */
        SDL_Texture *idle_texture;
        SDL_Texture *walking_texture;
        SDL_Texture *jumping_texture;
        SDL_Texture *attack_texture;
        SDL_Texture *attack2_texture;
        SDL_Texture *attack3_texture;
        SDL_Texture *block_texture;
        SDL_Texture *hurt_texture;
        SDL_Texture *death_texture;
        SDL_Texture *slide_texture;
        SDL_Texture *block_hurt_texture;
        SDL_Texture *pray_texture;
        SDL_Texture *down_attack_texture;
        SDL_Texture *reposition_texture;
        /* --- Repositioning ---*/
        Uint32 reposition_start_time; // Tracks how long to reposition for
        Uint32 last_reposition_time;
        Uint32 current_reposition_duration;
        /* --- Animation slicing --- */
        int frame_height;
        float frame_width; /* single frame width in the spritesheet */
        int frame_count;
        int frame_delay; /* ms between frames */
        int current_frame;
        Uint32 last_frame_time;

        SDL_Rect src_rect;
        SDL_Rect dest_rect;
    } Enemy;

    Enemy *create_enemy(SDL_Renderer *renderer, float x, float y);
    void destroy_enemy(Enemy *enemy);
    void handle_enemy_ai(Enemy *enemy, Player *player, Uint32 delta_time);
    void set_enemy_state(Enemy *enemy, EnemyState s);
    void update_enemy(Enemy *enemy, Uint32 delta_time); /* physics + anim timers only */
    void render_enemy(SDL_Renderer *renderer, Enemy *enemy);

#ifdef __cplusplus
}
#endif

#endif /* ENEMY_H */
