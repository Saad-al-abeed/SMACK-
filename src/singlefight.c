#include "singlefight.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

/* Tunables Mirrored from multifight.c */
#define ATTACK_RANGE 200.0f
#define DOWN_ATTACK_RANGE 50.0f
#define VERTICAL_RANGE 100.0f
#define ALLOWED_OVERLAP 150

/* ---- Setup / teardown ---- */
SingleFight *create_single_fight(void)
{
    SingleFight *fight = (SingleFight *)malloc(sizeof(SingleFight));
    if (!fight)
    {
        fprintf(stderr, "Failed to allocate memory for SingleFight\n");
        return NULL;
    }

    fight->fighter1 = (Warrior *)calloc(1, sizeof(Warrior));
    fight->fighter2 = (Warrior *)calloc(1, sizeof(Warrior));
    if (!fight->fighter1 || !fight->fighter2)
    {
        free(fight->fighter1);
        free(fight);
        return NULL;
    }

    fight->fighter1->health = MAX_HEALTH;
    fight->fighter2->health = MAX_HEALTH;

    fight->fight_over = false;
    fight->winner = 0;
    fight->restart_requested = false;
    return fight;
}

void destroy_single_fight(SingleFight *fight)
{
    if (!fight)
        return;
    free(fight->fighter1);
    free(fight->fighter2);
    free(fight);
}

/* ---- Update ---- */
void update_single_fight(SingleFight *fight, Player *p1, Enemy *en, Uint32 delta_time)
{
    if (!fight || !p1 || !en || fight->fight_over)
        return;

    // Use player's dynamic frame width for hitbox, matching multifight's corrected logic
    fight->fighter1->hitbox.x = (int)p1->x;
    fight->fighter1->hitbox.y = (int)p1->y;
    fight->fighter1->hitbox.w = 250;
    fight->fighter1->hitbox.h = (p1->state == PLAYER_SLIDE) ? 0 : 150;

    fight->fighter2->hitbox.x = (int)en->x;
    fight->fighter2->hitbox.y = (int)en->y;
    fight->fighter2->hitbox.w = 250;
    fight->fighter2->hitbox.h = 150;

    collision(fight, p1, en);
    combat(fight, p1, en);

    fighter1_state(fight->fighter1, p1, delta_time);
    update_enemy_state(fight->fighter2, en, delta_time);

    if (!fight->fight_over)
    {
        if (fight->fighter1->is_dead)
        {
            fight->winner = 2;
            fight->fight_over = true;
            fight->fight_end_time = SDL_GetTicks();
        }
        else if (fight->fighter2->is_dead)
        {
            fight->winner = 1;
            fight->fight_over = true;
            set_player_state(p1, PLAYER_PRAY); // Player prays on victory
            fight->fight_end_time = SDL_GetTicks();
        }
    }
}

void collision(SingleFight *fight, Player *p1, Enemy *en)
{
    SDL_Rect *b1 = &fight->fighter1->hitbox;
    SDL_Rect *b2 = &fight->fighter2->hitbox;

    if (!SDL_HasIntersection(b1, b2))
        return;

    int overlap_x = (p1->x < en->x) ? (b1->x + b1->w) - b2->x : (b2->x + b2->w) - b1->x;
    if (overlap_x <= ALLOWED_OVERLAP)
        return;

    bool p1Right = p1->velocity_x > 0, p1Left = p1->velocity_x < 0;
    bool enRight = en->velocity_x > 0, enLeft = en->velocity_x < 0;

    bool towards_each_other =
        (p1->x < en->x && p1Right && enLeft) ||
        (p1->x > en->x && p1Left && enRight);

    float push = (float)(overlap_x - ALLOWED_OVERLAP);

    if (towards_each_other)
    {
        p1->x += (p1->x < en->x ? -push * 0.5f : push * 0.5f);
        en->x += (p1->x < en->x ? push * 0.5f : -push * 0.5f);
        p1->velocity_x = en->velocity_x = 0;
    }
    else if (p1->velocity_x != 0 && en->velocity_x == 0)
    {
        p1->x += (p1->x < en->x ? -push : push);
        p1->velocity_x = 0;
    }
    else if (en->velocity_x != 0 && p1->velocity_x == 0)
    {
        en->x += (en->x < p1->x ? -push : push);
        en->velocity_x = 0;
    }

    if (p1->x < 0) p1->x = 0;
    if (en->x < 0) en->x = 0;

    // Use dynamic width for boundary checks
    int r1 = 1280 - (int)p1->frame_width;
    int r2 = 1280 - (int)en->frame_width;
    if (p1->x > r1) p1->x = r1;
    if (en->x > r2) en->x = r2;


    b1->x = (int)p1->x;
    b2->x = (int)en->x;
}

/* ---- Hit Detection Logic (Mirrored from multifight.c) ---- */
static bool facing_p1(Player *a, Enemy *d)
{
    return (a->direction == FACING_RIGHT && a->x < d->x) ||
           (a->direction == FACING_LEFT && a->x > d->x);
}

static bool p1_normal_attack_hits(Player *attacker, Enemy *defender)
{
    float dx = fabsf(attacker->x - defender->x);
    float dy = fabsf(attacker->y - defender->y);
    if (dy > VERTICAL_RANGE)
        return false;
    return (dx <= ATTACK_RANGE && facing_p1(attacker, defender));
}

static bool p1_down_attack_hits(Player *attacker, Enemy *defender)
{
    if (attacker->on_ground || attacker->y > defender->y || fabsf(attacker->y - defender->y) > VERTICAL_RANGE)
    {
        return false;
    }
    float dx = fabsf(attacker->x - defender->x);
    return (dx <= DOWN_ATTACK_RANGE);
}

// MODIFIED: This function now properly separates attack checks based on the attacker's state.
bool player1_attack_hit(Player *attacker, Enemy *defender)
{
    if (attacker->state == PLAYER_ATTACKING)
    {
        return p1_normal_attack_hits(attacker, defender);
    }
    else if (attacker->state == PLAYER_DOWN_ATTACK)
    {
        return p1_down_attack_hits(attacker, defender);
    }
    return false;
}

bool check_enemy_attack_hit(Enemy *attacker, Player *defender)
{
    float attack_range = 200.0f;
    float dx = fabsf(attacker->x - defender->x);
    float dy = fabsf(attacker->y - defender->y);

    bool facing_correct = (attacker->direction == R && attacker->x < defender->x) ||
                          (attacker->direction == L && attacker->x > defender->x);

    return (dx <= attack_range && dy <= VERTICAL_RANGE && facing_correct);
}

/* ---- Combat Logic (Mirrored from multifight.c) ---- */
void combat(SingleFight *fight, Player *p1, Enemy *en)
{
    if (fight->fighter1->is_dead || fight->fighter2->is_dead)
        return;

    /* Player attacks Enemy */
    if (p1->is_attacking &&
        !fight->fighter2->is_hurt && !fight->fighter2->is_dead &&
        player1_attack_hit(p1, en))
    {
        bool enemy_facing =
            ((p1->x < en->x && en->direction == L) ||
             (p1->x > en->x && en->direction == R));
        bool blocked = en->is_blocking && enemy_facing;

        if (blocked)
        {
            set_player_state(p1, PLAYER_BLOCK_HURT);
        }
        else
        {
            apply_damage_to_enemy(fight->fighter2, en, ATTACK_DAMAGE);
        }
    }

    /* Enemy attacks Player */
    if (en->is_attacking &&
        !fight->fighter1->is_hurt && !fight->fighter1->is_dead &&
        check_enemy_attack_hit(en, p1))
    {
        // A slide is invulnerable to the enemy's attack.
        bool slide_invulnerable = (p1->state == PLAYER_SLIDE);

        if(slide_invulnerable)
        {
            // Do nothing.
        }
        else
        {
            bool p1_facing =
                ((en->x < p1->x && p1->direction == FACING_LEFT) ||
                 (en->x > p1->x && p1->direction == FACING_RIGHT));
            bool blocked = p1->is_blocking && p1_facing;

            if (blocked)
            {
                // Enemy has no block-hurt state, so nothing happens to it.
            }
            else
            {
                damage_to_player1(fight->fighter1, p1, ATTACK_DAMAGE);
            }
        }
    }
}

/* ---- Damage & timers ---- */
void damage_to_player1(Warrior *fighter, Player *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0) fighter->health = 0;

    if (fighter->health == 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = SDL_GetTicks();
        set_player_state(player, PLAYER_DEATH);
    }
    else
    {
        fighter->is_hurt = true;
        fighter->hurt_start_time = SDL_GetTicks();
        set_player_state(player, PLAYER_HURT);
    }
}

void apply_damage_to_enemy(Warrior *fighter, Enemy *enemy, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0) fighter->health = 0;

    if (fighter->health == 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = SDL_GetTicks();
        set_enemy_state(enemy, ENEMY_DEATH);
    }
    else
    {
        fighter->is_hurt = true;
        fighter->hurt_start_time = SDL_GetTicks();
        set_enemy_state(enemy, ENEMY_HURT);
    }
}

// MODIFIED: This function now includes the attack animation timer logic from multifight.c
void fighter1_state(Warrior *fighter, Player *player, Uint32 delta_time)
{
    Uint32 now = SDL_GetTicks();
    
    // Attack animation timer
    if (player->is_attacking && (now - player->attack_start_time >= player->attack_duration))
    {
        player->is_attacking = false;
        if (player->state == PLAYER_ATTACKING || player->state == PLAYER_DOWN_ATTACK || player->state == PLAYER_BLOCK_HURT)
        {
            set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
        }
    }

    // Hurt timer
    if (fighter->is_hurt && now - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
    {
        fighter->is_hurt = false;
        if (player->state == PLAYER_HURT)
            set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
    }
}

void update_enemy_state(Warrior *fighter, Enemy *enemy, Uint32 delta_time)
{
    Uint32 now = SDL_GetTicks();
    if (fighter->is_hurt && now - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
    {
        fighter->is_hurt = false;
        if (enemy->state == ENEMY_HURT)
            set_enemy_state(enemy, ENEMY_IDLE);
    }
}

void health_bars(SDL_Renderer *renderer, SingleFight *fight)
{
    if (!fight || !renderer)
        return;

    int bar_width = 400, bar_height = 30, bar_y = 20;

    SDL_Rect p1_bg = {20, bar_y, bar_width, bar_height};
    SDL_Rect p1_health = {20, bar_y, (bar_width * fight->fighter1->health) / MAX_HEALTH, bar_height};

    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &p1_bg);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &p1_health);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &p1_bg);

    SDL_Rect p2_bg = {1280 - bar_width - 20, bar_y, bar_width, bar_height};
    SDL_Rect p2_health = {1280 - bar_width - 20, bar_y, (bar_width * fight->fighter2->health) / MAX_HEALTH, bar_height};

    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &p2_bg);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &p2_health);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &p2_bg);
}
void handle_single_fight_game_over_input(SingleFight *fight, const Uint8 *keystate)
{
    if (fight && fight->fight_over)
    {
        if (keystate[SDL_SCANCODE_RETURN])
        {
            fight->restart_requested = true;
        }
    }
}