#ifndef MULTI_FIGHT_H
#define MULTI_FIGHT_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h"
#include "player2.h"

// Fight system constants
#define MAX_HEALTH 100
#define ATTACK_DAMAGE 5
#define HURT_ANIMATION_DURATION 500   // milliseconds
#define DEATH_ANIMATION_DURATION 1000 // milliseconds
#define PLAYER_COLLISION_OFFSET 50    // pixels between players when colliding

// Fighter structure to track combat state
typedef struct
{
    int health;
    bool is_hurt;
    bool is_dead;
    Uint32 hurt_start_time;
    Uint32 death_start_time;
    SDL_Rect hitbox; // For collision detection
} Fighter;

// Multi-fight system structure
typedef struct
{
    Fighter *fighter1;
    Fighter *fighter2;
    bool fight_over;
    int winner; // 0 = no winner yet, 1 = player1, 2 = player2
} MultiFight;

// Function declarations
MultiFight *create_multi_fight(void);
void destroy_multi_fight(MultiFight *fight);
void update_multi_fight(MultiFight *fight, Player *player1, Player2 *player2, Uint32 delta_time);
void handle_collision(MultiFight *fight, Player *player1, Player2 *player2);
void handle_combat(MultiFight *fight, Player *player1, Player2 *player2);
void apply_damage_to_player1(Fighter *fighter, Player *player, int damage);
void apply_damage_to_player2(Fighter *fighter, Player2 *player, int damage);
void update_fighter1_state(Fighter *fighter, Player *player, Uint32 delta_time);
void update_fighter2_state(Fighter *fighter, Player2 *player, Uint32 delta_time);
bool check_player1_attack_hit(Player *attacker, Player2 *defender);
bool check_player2_attack_hit(Player2 *attacker, Player *defender);
void render_health_bars(SDL_Renderer *renderer, MultiFight *fight);

#endif // MULTI_FIGHT_H