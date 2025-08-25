#include "multifight.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define ALLOWED_OVERLAP 80 // pixels

MultiFight *create_multi_fight(void)
{
    MultiFight *fight = (MultiFight *)malloc(sizeof(MultiFight));
    if (!fight)
    {
        fprintf(stderr, "Failed to allocate memory for MultiFight\n");
        return NULL;
    }

    // Create fighter 1
    fight->fighter1 = (Fighter *)malloc(sizeof(Fighter));
    if (!fight->fighter1)
    {
        free(fight);
        return NULL;
    }
    fight->fighter1->health = MAX_HEALTH;
    fight->fighter1->is_hurt = false;
    fight->fighter1->is_dead = false;
    fight->fighter1->hurt_start_time = 0;
    fight->fighter1->death_start_time = 0;

    // Create fighter 2
    fight->fighter2 = (Fighter *)malloc(sizeof(Fighter));
    if (!fight->fighter2)
    {
        free(fight->fighter1);
        free(fight);
        return NULL;
    }
    fight->fighter2->health = MAX_HEALTH;
    fight->fighter2->is_hurt = false;
    fight->fighter2->is_dead = false;
    fight->fighter2->hurt_start_time = 0;
    fight->fighter2->death_start_time = 0;

    fight->fight_over = false;
    fight->winner = 0;

    return fight;
}

void destroy_multi_fight(MultiFight *fight)
{
    if (!fight)
        return;

    if (fight->fighter1)
        free(fight->fighter1);
    if (fight->fighter2)
        free(fight->fighter2);
    free(fight);
}

void update_multi_fight(MultiFight *fight, Player *player1, Player2 *player2, Uint32 delta_time)
{
    if (!fight || !player1 || !player2 || fight->fight_over)
        return;

    // Update hitboxes based on player positions
    fight->fighter1->hitbox.x = (int)player1->x;
    fight->fighter1->hitbox.y = (int)player1->y;
    fight->fighter1->hitbox.w = 250;
    fight->fighter1->hitbox.h = 150;

    fight->fighter2->hitbox.x = (int)player2->x;
    fight->fighter2->hitbox.y = (int)player2->y;
    fight->fighter2->hitbox.w = 250;
    fight->fighter2->hitbox.h = 150;

    // Handle collision between players
    handle_collision(fight, player1, player2);

    // Handle combat
    handle_combat(fight, player1, player2);

    // Update fighter states
    update_fighter1_state(fight->fighter1, player1, delta_time);
    update_fighter2_state(fight->fighter2, player2, delta_time);

    // Check for fight over
    if (fight->fighter1->is_dead && fight->winner == 0)
    {
        fight->winner = 2;
        fight->fight_over = true;
    }
    else if (fight->fighter2->is_dead && fight->winner == 0)
    {
        fight->winner = 1;
        fight->fight_over = true;
    }
}

void handle_collision(MultiFight *fight, Player *p1, Player2 *p2)
{
    SDL_Rect *b1 = &fight->fighter1->hitbox;
    SDL_Rect *b2 = &fight->fighter2->hitbox;

    if (!SDL_HasIntersection(b1, b2)) /* 0) */
        return;

    /* 1)  x-overlap in pixels (positive) ------------------------ */
    int overlap_x;
    if (p1->x < p2->x)
        overlap_x = (b1->x + b1->w) - b2->x; /* p1 left  */
    else
        overlap_x = (b2->x + b2->w) - b1->x; /* p2 left  */

    if (overlap_x <= ALLOWED_OVERLAP) /* 2) */
        return;                       /* small = ok */

    /* 3)  decide who is “pushing” ------------------------------- */
    bool p1Right = p1->velocity_x > 0;
    bool p1Left = p1->velocity_x < 0;
    bool p2Right = p2->velocity_x > 0;
    bool p2Left = p2->velocity_x < 0;

    bool towards_each_other =
        (p1->x < p2->x && p1Right && p2Left) ||
        (p1->x > p2->x && p1Left && p2Right);

    float push = (float)(overlap_x - ALLOWED_OVERLAP);

    if (towards_each_other)
    { /* both run in */
        p1->x += (p1->x < p2->x ? -push * 0.5f : push * 0.5f);
        p2->x += (p1->x < p2->x ? push * 0.5f : -push * 0.5f);
        p1->velocity_x = p2->velocity_x = 0;
    }
    else if (p1->velocity_x != 0 && p2->velocity_x == 0)
    { /* p1 pushing */
        p1->x += (p1->x < p2->x ? -push : push);
        p1->velocity_x = 0;
    }
    else if (p2->velocity_x != 0 && p1->velocity_x == 0)
    { /* p2 pushing */
        p2->x += (p2->x < p1->x ? -push : push);
        p2->velocity_x = 0;
    }
    /* same dir & both moving is allowed – already nudged enough */

    /* 4)  arena limits ----------------------------------------- */
    if (p1->x < 0)
        p1->x = 0;
    if (p2->x < 0)
        p2->x = 0;
    int r1 = 1280 - 250;
    if (p1->x > r1)
        p1->x = r1;
    int r2 = 1280 - 250;
    if (p2->x > r2)
        p2->x = r2;

    /* sync hit-boxes */
    b1->x = (int)p1->x;
    b2->x = (int)p2->x;
}

void handle_combat(MultiFight *fight, Player *p1, Player2 *p2)
{
    if (fight->fighter1->is_dead || fight->fighter2->is_dead)
        return;

    if (!SDL_HasIntersection(&fight->fighter1->hitbox,
                             &fight->fighter2->hitbox))
        return;

    /* -------- P1 attacks P2 ----------------------------------- */
    if (p1->is_attacking &&
        !fight->fighter2->is_hurt && !fight->fighter2->is_dead &&
        check_player1_attack_hit(p1, p2))
    {
        bool blocked = p2->is_blocking &&
                       ((p1->x < p2->x && p2->direction == LEFT) ||
                        (p1->x > p2->x && p2->direction == RIGHT));

        p1->is_attacking = false; /* one swing → done */

        if (blocked)
        {
            fight->fighter1->is_hurt = true;
            fight->fighter1->hurt_start_time = SDL_GetTicks();
            set_player_state(p1, PLAYER_HURT);
        }
        else
        {
            apply_damage_to_player2(fight->fighter2, p2, ATTACK_DAMAGE);
        }
    }

    /* -------- P2 attacks P1 ----------------------------------- */
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
            fight->fighter2->is_hurt = true;
            fight->fighter2->hurt_start_time = SDL_GetTicks();
            set_player2_state(p2, PLAYER2_HURT);
        }
        else
        {
            apply_damage_to_player1(fight->fighter1, p1, ATTACK_DAMAGE);
        }
    }
}

bool check_player1_attack_hit(Player *attacker, Player2 *defender)
{
    // Simple range check - adjust the attack range as needed
    float attack_range = 200.0f; // pixels
    float distance = fabs(attacker->x - defender->x);

    // Check if attacker is facing the defender
    bool facing_correct = false;
    if (attacker->direction == FACING_RIGHT && attacker->x < defender->x)
    {
        facing_correct = true;
    }
    else if (attacker->direction == FACING_LEFT && attacker->x > defender->x)
    {
        facing_correct = true;
    }

    // Attack hits if within range and facing correct direction
    return (distance <= attack_range && facing_correct);
}

bool check_player2_attack_hit(Player2 *attacker, Player *defender)
{
    // Simple range check - adjust the attack range as needed
    float attack_range = 200.0f; // pixels
    float distance = fabs(attacker->x - defender->x);

    // Check if attacker is facing the defender
    bool facing_correct = false;
    if (attacker->direction == RIGHT && attacker->x < defender->x)
    {
        facing_correct = true;
    }
    else if (attacker->direction == LEFT && attacker->x > defender->x)
    {
        facing_correct = true;
    }

    // Attack hits if within range and facing correct direction
    return (distance <= attack_range && facing_correct);
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

void update_fighter1_state(Fighter *fighter, Player *player, Uint32 delta_time)
{
    Uint32 current_time = SDL_GetTicks();

    // Handle hurt animation
    if (fighter->is_hurt && player->state == PLAYER_HURT)
    {
        if (current_time - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
        {
            fighter->is_hurt = false;
            set_player_state(player, PLAYER_IDLE);
        }
    }

    //Handle death animation
    // if (fighter->is_dead && player->state == PLAYER_DEATH)
    // {
    //     // Check if death animation is complete
    //     if (player->current_frame >= player->frame_count - 1)
    //     {
    //         player->current_frame = player->frame_count - 1;
    //         return;
    //     }
    // }
}

void update_fighter2_state(Fighter *fighter, Player2 *player, Uint32 delta_time)
{
    Uint32 current_time = SDL_GetTicks();

    // Handle hurt animation
    if (fighter->is_hurt && player->state == PLAYER2_HURT)
    {
        if (current_time - fighter->hurt_start_time >= HURT_ANIMATION_DURATION)
        {
            fighter->is_hurt = false;
            set_player2_state(player, PLAYER2_IDLE);
        }
    }

    // Handle death animation
    // if (fighter->is_dead && player->state == PLAYER2_DEATH)
    // {
    //     // Check if death animation is complete
    //     if (player->current_frame >= player->frame_count - 1)
    //     {
    //         player->current_frame = player->frame_count - 1;
    //         return;
    //     }
    // }
}

void render_health_bars(SDL_Renderer *renderer, MultiFight *fight)
{
    if (!fight || !renderer)
        return;

    // Health bar dimensions
    int bar_width = 400;
    int bar_height = 30;
    int bar_y = 20;

    // Player 1 health bar (left side)
    SDL_Rect p1_bg = {20, bar_y, bar_width, bar_height};
    SDL_Rect p1_health = {20, bar_y, (bar_width * fight->fighter1->health) / MAX_HEALTH, bar_height};

    // Background (red)
    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &p1_bg);

    // Health (green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &p1_health);

    // Border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &p1_bg);

    // Player 2 health bar (right side)
    SDL_Rect p2_bg = {1280 - bar_width - 20, bar_y, bar_width, bar_height};
    SDL_Rect p2_health = {1280 - bar_width - 20, bar_y, (bar_width * fight->fighter2->health) / MAX_HEALTH, bar_height};

    // Background (red)
    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &p2_bg);

    // Health (green)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &p2_health);

    // Border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &p2_bg);
}