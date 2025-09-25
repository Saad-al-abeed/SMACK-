#include "singlefight.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

/* Mirror multifight.c tunables */
#define ALLOWED_OVERLAP 150   /* match multi so push feels the same */
#define ATTACK_RANGE 200.0f   /* horizontal range for normal attacks */
#define VERTICAL_RANGE 100.0f /* max vertical separation to allow any attack */
#define HITBOX_W 250

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

    fight->fighter1->hitbox.x = (int)p1->x;
    fight->fighter1->hitbox.y = (int)p1->y;
    fight->fighter1->hitbox.w = HITBOX_W;
    fight->fighter1->hitbox.h = 150;

    fight->fighter2->hitbox.x = (int)en->x;
    fight->fighter2->hitbox.y = (int)en->y;
    fight->fighter2->hitbox.w = HITBOX_W;
    fight->fighter2->hitbox.h = 150;

    collision(fight, p1, en);
    combat(fight, p1, en); /* now mirrors multifight.c mechanics */

    fighter1_state(fight->fighter1, p1, delta_time);
    update_enemy_state(fight->fighter2, en, delta_time); /* enemy AI/state timing UNTOUCHED */

    if (!fight->fight_over)
    {
        if (fight->fighter1->is_dead)
        {
            fight->winner = 2;
            fight->fight_over = true;
        }
        else if (fight->fighter2->is_dead)
        {
            fight->winner = 1;
            fight->fight_over = true;
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

    if (p1->x < 0)
        p1->x = 0;
    if (en->x < 0)
        en->x = 0;
    int r = 1280 - HITBOX_W;
    if (p1->x > r)
        p1->x = r;
    if (en->x > r)
        en->x = r;

    b1->x = (int)p1->x;
    b2->x = (int)en->x;
}

/* ---- Player normal attack + facing (now also checks vertical distance) ---- */
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
        return false; /* must be within 100 px vertically */
    return (dx <= ATTACK_RANGE && facing_p1(attacker, defender));
}

/* ---- Player's vertical down attack (perfect vertical like multifight) ---- */
static bool p1_down_attack_hits(Player *attacker, Enemy *defender)
{
    if (attacker->state != PLAYER_DOWN_ATTACK)
        return false;
    if (attacker->on_ground)
        return false;

    float dx = fabsf(attacker->x - defender->x);
    if (dx != 0.0f)
        return false; /* EXACT vertical alignment (mirror multi) */
    if (fabsf(attacker->y - defender->y) > VERTICAL_RANGE)
        return false; /* obey 100 px vertical rule */

    return (attacker->y <= defender->y); /* attacker above defender */
}

bool player1_attack_hit(Player *attacker, Enemy *defender)
{
    if (p1_down_attack_hits(attacker, defender))
        return true;
    return p1_normal_attack_hits(attacker, defender);
}

/* ---- Enemy normal attack + facing (AI NOT TOUCHED) ---- */
bool check_enemy_attack_hit(Enemy *attacker, Player *defender)
{
    float attack_range = 200.0f;
    float dx = fabsf(attacker->x - defender->x);
    float dy = fabsf(attacker->y - defender->y);

    bool facing_correct = false;
    if (attacker->direction == R && attacker->x < defender->x)
        facing_correct = true;
    else if (attacker->direction == L && attacker->x > defender->x)
        facing_correct = true;

    /* Mirror the same vertical rule for consistency */
    return (dx <= attack_range && dy <= VERTICAL_RANGE && facing_correct);
}

/* ---- Combat: mirror multifight (no stand-still or ground→air gates; simple facing block) ---- */
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

        p1->is_attacking = false; /* same as multifight.c */

        if (blocked)
        {
            /* mirror multifight “attacker gets block-hurt” feel with a simple hurt feedback */
            fight->fighter1->is_hurt = true;
            fight->fighter1->hurt_start_time = SDL_GetTicks();
            set_player_state(p1, PLAYER_HURT);
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
        bool p1_facing =
            ((en->x < p1->x && p1->direction == FACING_LEFT) ||
             (en->x > p1->x && p1->direction == FACING_RIGHT));
        bool blocked = p1->is_blocking && p1_facing;

        en->is_attacking = false;

        if (blocked)
        {
            fight->fighter2->is_hurt = true;
            fight->fighter2->hurt_start_time = SDL_GetTicks();
            set_enemy_state(en, ENEMY_HURT);
        }
        else
        {
            damage_to_player1(fight->fighter1, p1, ATTACK_DAMAGE);
        }
    }
}

/* ---- Damage & timers (force death immediately at 0, like multifight) ---- */
void damage_to_player1(Warrior *fighter, Player *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0)
        fighter->health = 0;

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
    if (fighter->health < 0)
        fighter->health = 0;

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

void fighter1_state(Warrior *fighter, Player *player, Uint32 delta_time)
{
    Uint32 now = SDL_GetTicks();
    if (fighter->is_hurt && now - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
    {
        fighter->is_hurt = false;
        if (player->state == PLAYER_HURT)
            set_player_state(player, PLAYER_IDLE);
    }
}

void update_enemy_state(Warrior *fighter, Enemy *enemy, Uint32 delta_time)
{
    /* Enemy AI/state timing — left unchanged */
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
