#include "multifight.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

/* Tunables */
#define ATTACK_RANGE 200.0f   /* horizontal range for normal attacks */
#define VERTICAL_RANGE 100.0f /* max vertical separation to allow any attack */
#define ALLOWED_OVERLAP 150   /* push-back tolerance for movement collision */
#define HITBOX_W 250
#define HITBOX_H 150

/* ---------- Helpers ---------- */
static inline float absf(float v) { return v < 0 ? -v : v; }

/* Facing checks (unchanged from your prior setup) */
static inline bool facing_p1(Player *a, Player2 *d)
{
    return (a->direction == FACING_RIGHT && a->x < d->x) ||
           (a->direction == FACING_LEFT && a->x > d->x);
}
static inline bool facing_p2(Player2 *a, Player *d)
{
    return (a->direction == RIGHT && a->x < d->x) ||
           (a->direction == LEFT && a->x > d->x);
}

/* --- Down attack: ONLY when perfectly vertical (dx == 0), airborne, and above --- */
static bool down_attack_hits_player1(Player *attacker, Player2 *defender)
{
    if (attacker->state != PLAYER_DOWN_ATTACK)
        return false;
    if (attacker->on_ground)
        return false;
    float dx = absf(attacker->x - defender->x);
    if (dx != 0.0f)
        return false; /* EXACT vertical alignment, per your spec */
    if (absf(attacker->y - defender->y) > VERTICAL_RANGE)
        return false;                    /* obey global vertical rule */
    return (attacker->y <= defender->y); /* attacker above defender */
}
static bool down_attack_hits_player2(Player2 *attacker, Player *defender)
{
    if (attacker->state != PLAYER2_DOWN_ATTACK)
        return false;
    if (attacker->on_ground)
        return false;
    float dx = absf(attacker->x - defender->x);
    if (dx != 0.0f)
        return false;
    if (absf(attacker->y - defender->y) > VERTICAL_RANGE)
        return false;
    return (attacker->y <= defender->y);
}

/* --- Normal attacks: must pass BOTH horizontal & vertical thresholds + facing --- */
static bool p1_normal_attack_hits(Player *attacker, Player2 *defender)
{
    float dx = absf(attacker->x - defender->x);
    float dy = absf(attacker->y - defender->y);
    if (dy > VERTICAL_RANGE)
        return false; /* global “>100 px ⇒ no register” */
    return (dx <= ATTACK_RANGE && facing_p1(attacker, defender));
}
static bool p2_normal_attack_hits(Player2 *attacker, Player *defender)
{
    float dx = absf(attacker->x - defender->x);
    float dy = absf(attacker->y - defender->y);
    if (dy > VERTICAL_RANGE)
        return false;
    return (dx <= ATTACK_RANGE && facing_p2(attacker, defender));
}

bool check_player1_attack_hit(Player *attacker, Player2 *defender)
{
    /* Down-attack first (strict vertical), else normal check */
    if (down_attack_hits_player1(attacker, defender))
        return true;
    return p1_normal_attack_hits(attacker, defender);
}
bool check_player2_attack_hit(Player2 *attacker, Player *defender)
{
    if (down_attack_hits_player2(attacker, defender))
        return true;
    return p2_normal_attack_hits(attacker, defender);
}

/* ---------- Core object lifecycle ---------- */
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

/* ---------- Movement collision (kept same style as before) ---------- */
void handle_collision(MultiFight *fight, Player *p1, Player2 *p2)
{
    SDL_Rect *b1 = &fight->fighter1->hitbox;
    SDL_Rect *b2 = &fight->fighter2->hitbox;

    if (!SDL_HasIntersection(b1, b2))
        return;

    int overlap_x = (p1->x < p2->x) ? (b1->x + b1->w) - b2->x : (b2->x + b2->w) - b1->x;
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
    int r = 1280 - HITBOX_W;
    if (p1->x > r)
        p1->x = r;
    if (p2->x > r)
        p2->x = r;

    b1->x = (int)p1->x;
    b2->x = (int)p2->x;
}

/* ---------- Damage application (force death immediately at 0) ---------- */
void apply_damage_to_player1(Fighter *fighter, Player *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0)
        fighter->health = 0;

    if (fighter->health == 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = SDL_GetTicks();
        set_player_state(player, PLAYER_DEATH); /* force death state immediately */
        return;
    }

    fighter->is_hurt = true;
    fighter->hurt_start_time = SDL_GetTicks();
    set_player_state(player, PLAYER_HURT);
}

void apply_damage_to_player2(Fighter *fighter, Player2 *player, int damage)
{
    fighter->health -= damage;
    if (fighter->health < 0)
        fighter->health = 0;

    if (fighter->health == 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = SDL_GetTicks();
        set_player2_state(player, PLAYER2_DEATH); /* force death state immediately */
        return;
    }

    fighter->is_hurt = true;
    fighter->hurt_start_time = SDL_GetTicks();
    set_player2_state(player, PLAYER2_HURT);
}

/* ---------- Hurt timers (state-independent clearing) ---------- */
void update_fighter1_state(Fighter *fighter, Player *player)
{
    Uint32 now = SDL_GetTicks();

    /* Safety: if health already 0 but not flagged dead for any reason, enforce death */
    if (!fighter->is_dead && fighter->health <= 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = now;
        set_player_state(player, PLAYER_DEATH);
    }

    if (fighter->is_hurt && (now - fighter->hurt_start_time) >= HURT_ANIMATION_DURATION)
    {
        fighter->is_hurt = false;
        if (player->state == PLAYER_HURT)
            set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
    }
}

void update_fighter2_state(Fighter *fighter, Player2 *player)
{
    Uint32 now = SDL_GetTicks();

    if (!fighter->is_dead && fighter->health <= 0)
    {
        fighter->is_dead = true;
        fighter->death_start_time = now;
        set_player2_state(player, PLAYER2_DEATH);
    }

    if (fighter->is_hurt && (now - fighter->hurt_start_time) >= HURT_ANIMATION_DURATION)
    {
        fighter->is_hurt = false;
        if (player->state == PLAYER2_HURT)
            set_player2_state(player, player->on_ground ? PLAYER2_IDLE : PLAYER2_JUMPING);
    }
}

/* ---------- Combat ---------- */
void handle_combat(MultiFight *fight, Player *p1, Player2 *p2)
{
    if (fight->fighter1->is_dead || fight->fighter2->is_dead)
        return;

    /* ---- Player 1 attacks Player 2 ---- */
    if (p1->is_attacking &&
        !fight->fighter2->is_hurt && !fight->fighter2->is_dead &&
        check_player1_attack_hit(p1, p2))
    {
        /* Optional: keep facing-based block. If you want to ignore blocks, remove this chunk. */
        bool defender_facing =
            ((p1->x < p2->x && p2->direction == LEFT) ||
             (p1->x > p2->x && p2->direction == RIGHT));
        bool blocked = p2->is_blocking && defender_facing;

        p1->is_attacking = false;

        if (blocked)
        {
            set_player_state(p1, PLAYER_BLOCK_HURT); /* feedback to attacker */
        }
        else
        {
            apply_damage_to_player2(fight->fighter2, p2, ATTACK_DAMAGE);
        }
    }

    /* ---- Player 2 attacks Player 1 ---- */
    if (p2->is_attacking &&
        !fight->fighter1->is_hurt && !fight->fighter1->is_dead &&
        check_player2_attack_hit(p2, p1))
    {
        bool defender_facing =
            ((p2->x < p1->x && p1->direction == FACING_LEFT) ||
             (p2->x > p1->x && p1->direction == FACING_RIGHT));
        bool blocked = p1->is_blocking && defender_facing;

        p2->is_attacking = false;

        if (blocked)
        {
            set_player2_state(p2, PLAYER2_BLOCK_HURT);
        }
        else
        {
            apply_damage_to_player1(fight->fighter1, p1, ATTACK_DAMAGE);
        }
    }
}

/* ---------- Public frame update & UI ---------- */
void update_multi_fight(MultiFight *fight, Player *p1, Player2 *p2, Uint32 delta_time)
{
    if (!fight || !p1 || !p2)
        return;

    /* Hitboxes */
    fight->fighter1->hitbox.x = (int)p1->x;
    fight->fighter1->hitbox.y = (int)p1->y;
    fight->fighter1->hitbox.w = HITBOX_W;
    fight->fighter1->hitbox.h = HITBOX_H;

    fight->fighter2->hitbox.x = (int)p2->x;
    fight->fighter2->hitbox.y = (int)p2->y;
    fight->fighter2->hitbox.w = HITBOX_W;
    fight->fighter2->hitbox.h = HITBOX_H;

    handle_collision(fight, p1, p2);
    handle_combat(fight, p1, p2);

    update_fighter1_state(fight->fighter1, p1);
    update_fighter2_state(fight->fighter2, p2);

    /* Winner / end-of-fight bookkeeping (optional pray trigger if you use it elsewhere) */
    if (!fight->fight_over)
    {
        if (fight->fighter1->is_dead)
        {
            fight->fight_over = true;
            fight->winner = 2; /* set_player2_state(p2, PLAYER2_PRAY); */
        }
        else if (fight->fighter2->is_dead)
        {
            fight->fight_over = true;
            fight->winner = 1; /* set_player_state(p1, PLAYER_PRAY); */
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
