#include "multifight.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define ALLOWED_OVERLAP 80 /* pixels */

/* Helper: detect slide state to zero-height hitbox */
static bool is_player_sliding(const Player *p) { return p->state == PLAYER_SLIDE; }
static bool is_player2_sliding(const Player2 *p) { return p->state == PLAYER2_SLIDE; }

/* Helper: down-attack hit check (aerial, vertical strike) */
static bool down_attack_hits_player1(Player *attacker, Player2 *defender)
{
    if (attacker->state != PLAYER_DOWN_ATTACK)
        return false;
    if (attacker->on_ground)
        return false;
    float dx = fabsf(attacker->x - defender->x);
    /* PLACEHOLDER tuning: narrow x window, attacker above defender */
    return (dx <= 120.0f && attacker->y <= defender->y);
}
static bool down_attack_hits_player2(Player2 *attacker, Player *defender)
{
    if (attacker->state != PLAYER2_DOWN_ATTACK)
        return false;
    if (attacker->on_ground)
        return false;
    float dx = fabsf(attacker->x - defender->x);
    return (dx <= 120.0f && attacker->y <= defender->y);
}

MultiFight *create_multi_fight(void)
{
    MultiFight *fight = (MultiFight *)malloc(sizeof(MultiFight));
    if (!fight)
    {
        fprintf(stderr, "Failed to allocate MultiFight\n");
        return NULL;
    }

    fight->fighter1 = (Fighter *)calloc(1, sizeof(Fighter));
    fight->fighter2 = (Fighter *)calloc(1, sizeof(Fighter));
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

void destroy_multi_fight(MultiFight *fight)
{
    if (!fight)
        return;
    free(fight->fighter1);
    free(fight->fighter2);
    free(fight);
}

void update_multi_fight(MultiFight *fight, Player *player1, Player2 *player2, Uint32 delta_time)
{
    if (!fight || !player1 || !player2)
        return;

    /* Update hitboxes – zero height when sliding */
    fight->fighter1->hitbox.x = (int)player1->x;
    fight->fighter1->hitbox.y = (int)player1->y;
    fight->fighter1->hitbox.w = 250;
    fight->fighter1->hitbox.h = is_player_sliding(player1) ? 0 : 150;

    fight->fighter2->hitbox.x = (int)player2->x;
    fight->fighter2->hitbox.y = (int)player2->y;
    fight->fighter2->hitbox.w = 250;
    fight->fighter2->hitbox.h = is_player2_sliding(player2) ? 0 : 150;

    /* Don’t push around during slide (no vertical height) */
    handle_collision(fight, player1, player2);

    /* Combat */
    handle_combat(fight, player1, player2);

    /* State timers: clear timed hurt */
    update_fighter1_state(fight->fighter1, player1, delta_time);
    update_fighter2_state(fight->fighter2, player2, delta_time);

    /* Check for deaths and trigger winner PRAY once */
    if (!fight->fight_over)
    {
        if (fight->fighter1->is_dead)
        {
            fight->fight_over = true;
            fight->winner = 2;
            set_player2_state(player2, PLAYER2_PRAY); /* play once, then to idle in player2.c */
        }
        else if (fight->fighter2->is_dead)
        {
            fight->fight_over = true;
            fight->winner = 1;
            set_player_state(player1, PLAYER_PRAY); /* play once, then to idle in player.c  */
        }
    }
}

void handle_collision(MultiFight *fight, Player *p1, Player2 *p2)
{
    SDL_Rect *b1 = &fight->fighter1->hitbox;
    SDL_Rect *b2 = &fight->fighter2->hitbox;

    if (!SDL_HasIntersection(b1, b2))
        return;

    int overlap_x;
    if (p1->x < p2->x)
        overlap_x = (b1->x + b1->w) - b2->x;
    else
        overlap_x = (b2->x + b2->w) - b1->x;

    if (overlap_x <= ALLOWED_OVERLAP)
        return;

    bool p1Right = p1->velocity_x > 0, p1Left = p1->velocity_x < 0;
    bool p2Right = p2->velocity_x > 0, p2Left = p2->velocity_x < 0;

    bool towards_each_other =
        (p1->x < p2->x && p1Right && p2Left) ||
        (p1->x > p2->x && p1Left && p2Right);

    float push = (float)(overlap_x - ALLOWED_OVERLAP);

    if (towards_each_other)
    {
        p1->x += (p1->x < p2->x ? -push * 0.5f : push * 0.5f);
        p2->x += (p1->x < p2->x ? push * 0.5f : -push * 0.5f);
        p1->velocity_x = p2->velocity_x = 0;
    }
    else if (p1->velocity_x != 0 && p2->velocity_x == 0)
    {
        p1->x += (p1->x < p2->x ? -push : push);
        p1->velocity_x = 0;
    }
    else if (p2->velocity_x != 0 && p1->velocity_x == 0)
    {
        p2->x += (p2->x < p1->x ? -push : push);
        p2->velocity_x = 0;
    }

    if (p1->x < 0)
        p1->x = 0;
    if (p2->x < 0)
        p2->x = 0;
    int r = 1280 - 250;
    if (p1->x > r)
        p1->x = r;
    if (p2->x > r)
        p2->x = r;

    b1->x = (int)p1->x;
    b2->x = (int)p2->x;
}

/* Existing helpers (range + facing) with DOWN-ATTACK support added */
static bool facing_player1(Player *a, Player2 *d)
{
    return (a->direction == FACING_RIGHT && a->x < d->x) ||
           (a->direction == FACING_LEFT && a->x > d->x);
}
static bool facing_player2(Player2 *a, Player *d)
{
    return (a->direction == RIGHT && a->x < d->x) ||
           (a->direction == LEFT && a->x > d->x);
}

bool check_player1_attack_hit(Player *attacker, Player2 *defender)
{
    float attack_range = 200.0f;
    float distance = fabsf(attacker->x - defender->x);

    if (down_attack_hits_player1(attacker, defender))
        return true;

    bool facing_correct = facing_player1(attacker, defender);
    return (distance <= attack_range && facing_correct);
}

bool check_player2_attack_hit(Player2 *attacker, Player *defender)
{
    float attack_range = 200.0f;
    float distance = fabsf(attacker->x - defender->x);

    if (down_attack_hits_player2(attacker, defender))
        return true;

    bool facing_correct = facing_player2(attacker, defender);
    return (distance <= attack_range && facing_correct);
}

void handle_combat(MultiFight *fight, Player *p1, Player2 *p2)
{
    if (fight->fighter1->is_dead || fight->fighter2->is_dead)
        return;

    /* P1 attacks P2 */
    if (p1->is_attacking &&
        !fight->fighter2->is_hurt && !fight->fighter2->is_dead &&
        check_player1_attack_hit(p1, p2))
    {
        bool blocked = p2->is_blocking &&
                       ((p1->x < p2->x && p2->direction == LEFT) ||
                        (p1->x > p2->x && p2->direction == RIGHT));

        p1->is_attacking = false;

        if (blocked)
        {
            /* NEW: block-hurt (no real hurt flag) */
            set_player_state(p1, PLAYER_BLOCK_HURT);
        }
        else
        {
            apply_damage_to_player2(fight->fighter2, p2, ATTACK_DAMAGE);
        }
    }

    /* P2 attacks P1 */
    if (p2->is_attacking &&
        !fight->fighter1->is_hurt && !fight->fighter1->is_dead &&
        check_player2_attack_hit(p2, p1))
    {
        bool blocked = p1->is_blocking &&
                       ((p2->x < p1->x && p1->direction == FACING_LEFT) ||
                        (p2->x > p1->x && p1->direction == FACING_RIGHT));

        p2->is_attacking = false;

        if (blocked)
        {
            /* NEW: block-hurt (no real hurt flag) */
            set_player2_state(p2, PLAYER2_BLOCK_HURT);
        }
        else
        {
            apply_damage_to_player1(fight->fighter1, p1, ATTACK_DAMAGE);
        }
    }
}

void apply_damage_to_player1(Fighter *fighter, Player *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0)
        fighter->health = 0;

    if (fighter->health <= 0)
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

void apply_damage_to_player2(Fighter *fighter, Player2 *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0)
        fighter->health = 0;

    if (fighter->health <= 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = SDL_GetTicks();
        set_player2_state(player, PLAYER2_DEATH);
    }
    else
    {
        fighter->is_hurt = true;
        fighter->hurt_start_time = SDL_GetTicks();
        set_player2_state(player, PLAYER2_HURT);
    }
}

/* Timed hurt clear (don’t depend on render state) */
void update_fighter1_state(Fighter *fighter, Player *player, Uint32 delta_time)
{
    Uint32 now = SDL_GetTicks();
    if (fighter->is_hurt)
    {
        if (now - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
        {
            fighter->is_hurt = false;
            if (player->state == PLAYER_HURT)
                set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
        }
    }
}

void update_fighter2_state(Fighter *fighter, Player2 *player, Uint32 delta_time)
{
    Uint32 now = SDL_GetTicks();
    if (fighter->is_hurt)
    {
        if (now - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
        {
            fighter->is_hurt = false;
            if (player->state == PLAYER2_HURT)
                set_player2_state(player, player->on_ground ? PLAYER2_IDLE : PLAYER2_JUMPING);
        }
    }
}

void render_health_bars(SDL_Renderer *renderer, MultiFight *fight)
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
