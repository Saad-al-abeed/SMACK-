#ifndef SINGLE_FIGHT_H
#define SINGLE_FIGHT_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "player.h"
#include "enemy.h"

// Fight system constants
#define MAX_HEALTH 100
#define ATTACK_DAMAGE 5
#define HURT_ANIMATION_DURATION 500   // milliseconds
#define DEATH_ANIMATION_DURATION 1000 // milliseconds
#define PLAYER_COLLISION_OFFSET 50    // pixels between players when colliding

// Warrior structure to track combat state
typedef struct
{
    int health;
    bool is_hurt;
    bool is_dead;
    Uint32 hurt_start_time;
    Uint32 death_start_time;
    SDL_Rect hitbox; // For collision detection
} Warrior;

// Single-fight system structure
typedef struct
{
    Warrior *fighter1;
    Warrior *fighter2;
    bool fight_over;
    int winner; // 0 = no winner yet, 1 = player1, 2 = enemy
} SingleFight;

// Function declarations
SingleFight *create_single_fight(void);
void destroy_single_fight(SingleFight *fight);
void update_single_fight(SingleFight *fight, Player *player1, Enemy *enemy, Uint32 delta_time);
void collision(SingleFight *fight, Player *player1, Enemy *enemy);
void combat(SingleFight *fight, Player *player1, Enemy *enemy);
void damage_to_player1(Warrior *fighter, Player *player, int damage, Enemy *enemy);
void apply_damage_to_enemy(Warrior *fighter, Enemy *player, int damage);
void fighter1_state(Warrior *fighter, Player *player, Uint32 delta_time);
void update_enemy_state(Warrior *fighter, Enemy *player, Uint32 delta_time, SingleFight *s);
bool player1_attack_hit(Player *attacker, Enemy *defender);
bool check_enemy_attack_hit(Enemy *attacker, Player *defender);
void health_bars(SDL_Renderer *renderer, SingleFight *fight);

#endif // SINGLE_FIGHT_H