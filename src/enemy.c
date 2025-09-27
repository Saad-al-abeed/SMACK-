#include "enemy.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

/* Keep these consistent with player */
#define GROUND_LEVEL 375
#define ENEMY_HEIGHT 258

/* Placeholders – mirror the player defaults (tune to your assets) */
#define DEFAULT_SPEED 450.0f
#define DEFAULT_JUMP_FORCE -900.0f
#define DEFAULT_GRAVITY 1500.0f

#define DEFAULT_ATTACK_DURATION_MS 500
#define DEFAULT_BLOCK_HURT_DURATION 300

/* ---------- helpers ---------- */
static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *s = SDL_LoadBMP(path);
    if (!s)
    {
        fprintf(stderr, "Enemy: failed to load BMP %s: %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, 255, 0, 255));
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    if (!t)
        fprintf(stderr, "Enemy: create texture from %s failed: %s\n", path, SDL_GetError());
    return t;
}

static void tex_dims(SDL_Texture *t, int *w, int *h)
{
    if (!t)
    {
        *w = *h = 0;
        return;
    }
    SDL_QueryTexture(t, NULL, NULL, w, h);
}

/* ---------- public API ---------- */

Enemy *create_enemy(SDL_Renderer *renderer, float x, float y)
{
    Enemy *e = (Enemy *)calloc(1, sizeof(Enemy));
    if (!e)
        return NULL;

    /* Physics */
    e->x = x;
    e->y = y;
    e->velocity_x = 0.0f;
    e->velocity_y = 0.0f;
    e->speed = DEFAULT_SPEED;
    e->jump_force = DEFAULT_JUMP_FORCE;
    e->gravity = DEFAULT_GRAVITY;
    e->on_ground = 0;
    e->x = x;
    e->y = y;
    e->speed = 350.0f;
    e->gravity = 1500.0f;
    e->attack_duration = 500; // ms
    e->direction = L;
    e->block_start_time = 0;

    /* Combat/anim timers */
    e->is_attacking = 0;
    e->is_blocking = 0;
    e->attack_start_time = 0;
    e->attack_duration = DEFAULT_ATTACK_DURATION_MS;
    e->current_attack = 0;
    e->block_hurt_start_time = 0;
    e->block_hurt_duration = DEFAULT_BLOCK_HURT_DURATION;

    e->health = 100;
    e->is_dead = 0;
    e->death_start_time = 0;

    e->direction = R;
    e->state = ENEMY_IDLE;

    /* --- Load textures (placeholder paths; mirror your player paths) --- */
    e->idle_texture = load_texture(renderer, "assets/textures/Final/Idle_h258_w516.bmp");
    e->walking_texture = load_texture(renderer, "assets/textures/Final/Run_h258_w516.bmp");
    e->jumping_texture = load_texture(renderer, "assets/textures/Final/nor_jmp_h258_w516.bmp");
    e->attack_texture = load_texture(renderer, "assets/textures/Final/atk1.bmp");
    e->attack2_texture = load_texture(renderer, "assets/textures/Final/atk3.bmp");
    e->attack3_texture = load_texture(renderer, "assets/textures/Final/atk4.bmp");
    e->block_texture = load_texture(renderer, "assets/textures/Final/crouch_idle-sheet.bmp");
    e->hurt_texture = load_texture(renderer, "assets/textures/Final/Hurt-sheet.bmp"); // Add this
    e->death_texture = load_texture(renderer, "assets/textures/Final/Dth_h258_w516.bmp");
    e->slide_texture = load_texture(renderer, "assets/textures/Final/Slide-sheet.bmp");
    e->block_hurt_texture = load_texture(renderer, "assets/textures/Final/blockhurt.bmp");
    e->pray_texture = load_texture(renderer, "assets/textures/Final/pray_h258_w516.bmp");
    e->down_attack_texture = load_texture(renderer, "assets/textures/jmph258w516.bmp");
    e->reposition_texture = load_texture(renderer, "assets/textures/Final/Run_h258_w516.bmp");

    /* Basic anim setup from idle */
    e->frame_height = ENEMY_HEIGHT;
    e->current_frame = 0;
    e->last_frame_time = SDL_GetTicks();

    int tw = 0, th = 0;
    tex_dims(e->idle_texture, &tw, &th);
    e->frame_count = (tw > 0) ? 6 : 1; /* placeholder; you’ll match to player */
    e->frame_delay = 100;
    e->frame_width = (e->frame_count > 0) ? (float)tw / (float)e->frame_count : (float)tw;

    e->src_rect = (SDL_Rect){0, 0, (int)e->frame_width, e->frame_height};
    e->dest_rect = (SDL_Rect){(int)e->x, (int)e->y, (int)e->frame_width, e->frame_height};

    set_enemy_state(e, ENEMY_IDLE);
    return e;
}

void destroy_enemy(Enemy *e)
{
    if (!e)
        return;
    SDL_Texture **txs[] = {
        &e->idle_texture, &e->walking_texture, &e->jumping_texture,
        &e->attack_texture, &e->attack2_texture, &e->attack3_texture,
        &e->block_texture, &e->hurt_texture, &e->death_texture,
        &e->slide_texture, &e->block_hurt_texture, &e->pray_texture, &e->down_attack_texture, &e->reposition_texture};
    for (size_t i = 0; i < sizeof(txs) / sizeof(txs[0]); ++i)
        if (*txs[i])
            SDL_DestroyTexture(*txs[i]);
    free(e);
}

void handle_enemy_ai(Enemy *enemy, Player *player, Uint32 delta_time)
{
    // --- Initial checks for death, etc. remain the same ---
    if (!enemy || !player || enemy->state == ENEMY_HURT || enemy->state == ENEMY_DEATH) {
        enemy->velocity_x = 0;
        return;
    }
    if (player->state == PLAYER_DEATH) {
        enemy->velocity_x = 0;
        set_enemy_state(enemy, ENEMY_IDLE);
        return;
    }

    // --- Senses ---
    float distance_x = player->x - enemy->x;
    float abs_distance = fabs(distance_x);
    enemy->direction = (distance_x > 0) ? R : L;
    bool is_player_facing_enemy = ((player->direction == FACING_RIGHT && distance_x < 0) ||
                                 (player->direction == FACING_LEFT && distance_x > 0));

    // --- AI Behavior Constants ---
    const int ATTACK_RANGE = 200;
    const int REPOSITION_TRIGGER_DISTANCE = 550;

    // --- NEW: Define ranges for our random values ---
    const Uint32 MIN_REPOSITION_DURATION = 400;  // 0.4 seconds
    const Uint32 MAX_REPOSITION_DURATION = 700;  // 0.7 seconds
    const Uint32 MIN_REPOSITION_COOLDOWN = 2500; // 2.5 seconds
    const Uint32 MAX_REPOSITION_COOLDOWN = 4000; // 4.0 seconds

    // --- Priority 1: Handle Ongoing Timed Actions ---
    if (enemy->is_attacking) {
        if (SDL_GetTicks() - enemy->attack_start_time < enemy->attack_duration) {
            enemy->velocity_x = 0;
            return;
        }
        enemy->is_attacking = false;
    }
    
    if (enemy->state == ENEMY_REPOSITIONING) {
        // Use the duration we stored when the action began
        if (SDL_GetTicks() - enemy->reposition_start_time < enemy->current_reposition_duration) {
            enemy->velocity_x = (enemy->direction == R) ? -enemy->speed * 0.7f : enemy->speed * 0.7f;
            return;
        } else {
            set_enemy_state(enemy, ENEMY_IDLE);
            enemy->velocity_x = 0;
            return;
        }
    }

    // --- Priority 2: DEFEND ---
    if (player->is_attacking && is_player_facing_enemy && abs_distance < ATTACK_RANGE + 50) {
        enemy->velocity_x = 0;
        set_enemy_state(enemy, ENEMY_BLOCKING);
        return;
    }

    // --- Priority 3: ATTACK ---
    bool player_is_vulnerable = (!player->is_blocking || !is_player_facing_enemy);
    if (abs_distance <= ATTACK_RANGE && player_is_vulnerable) {
        enemy->velocity_x = 0;
        enemy->is_attacking = true;
        enemy->current_attack = rand() % 3;
        enemy->attack_start_time = SDL_GetTicks();
        set_enemy_state(enemy, ENEMY_ATTACKING);
        return;
    }
    
    // --- Priority 4: DECIDE TO REPOSITION (NEW) ---
    // If the player gets too close AND our reposition ability is not on cooldown...
    Uint32 random_cooldown = (rand() % (MAX_REPOSITION_COOLDOWN - MIN_REPOSITION_COOLDOWN + 1)) + MIN_REPOSITION_COOLDOWN;

    if (abs_distance < REPOSITION_TRIGGER_DISTANCE && (SDL_GetTicks() - enemy->last_reposition_time > random_cooldown)) {
        // ...then start repositioning.
        enemy->reposition_start_time = SDL_GetTicks();
        enemy->last_reposition_time = enemy->reposition_start_time;


        
        // NEW: Calculate and store a random duration for THIS specific back-dash
        enemy->current_reposition_duration = (rand() % (MAX_REPOSITION_DURATION - MIN_REPOSITION_DURATION + 1)) + MIN_REPOSITION_DURATION;

        set_enemy_state(enemy, ENEMY_REPOSITIONING);
        return;
    }

    // --- Priority 5: CHASE (Default Action) ---
    // If no other actions were taken, chase the player.
    // This now works because repositioning is a temporary state and won't block it forever.
    if (abs_distance > ATTACK_RANGE) { // Only chase if outside of attack range
        enemy->velocity_x = (enemy->direction == R) ? enemy->speed : -enemy->speed;
        set_enemy_state(enemy, ENEMY_WALKING);
    } else {
        // If we are in attack range but can't attack (e.g., player is blocking), just stay idle.
        enemy->velocity_x = 0;
        set_enemy_state(enemy, ENEMY_IDLE);
    }
}

void set_enemy_state(Enemy *e, EnemyState s)
{
    if (!e)
        return;
    if (e->state == s && s != ENEMY_ATTACKING && s != ENEMY_DOWN_ATTACK)
        return;

    e->state = s;
    e->current_frame = 0;
    e->last_frame_time = SDL_GetTicks();

    SDL_Texture *t = NULL;
    int tw = 0, th = 0;

    /* Mirror the player's frame counts & delays (PLACEHOLDERS below; change to match player.c) */
    switch (s)
    {
    case ENEMY_IDLE:
        t = e->idle_texture;
        e->frame_count = 8;
        e->frame_delay = 100;
        break;
    case ENEMY_WALKING:
        t = e->walking_texture;
        e->frame_count = 8;
        e->frame_delay = 90;
        break;
    case ENEMY_REPOSITIONING:
        t = e->reposition_texture;
        e->frame_count = 8;
        e->frame_delay = 90;
        break;
    case ENEMY_JUMPING:
        t = e->jumping_texture;
        e->frame_count = 6;
        e->frame_delay = 100;
        break;

    case ENEMY_ATTACKING:
        t = (e->current_attack == 0 ? e->attack_texture : (e->current_attack == 1 ? e->attack2_texture : e->attack3_texture));
        /* Adjust counts/delays per attack index to match player exactly */
        if (e->current_attack == 1)
        {
            e->frame_count = 4;
            e->frame_delay = 150;
        }
        else
        {
            e->frame_count = 6;
            e->frame_delay = 80;
        }
        break;

    case ENEMY_BLOCKING:
        t = e->block_texture;
        e->frame_count = 1;
        e->frame_delay = 120;
        break;
    case ENEMY_HURT:
        t = e->hurt_texture;
        e->frame_count = 3;
        e->frame_delay = 120;
        break;
    case ENEMY_DEATH:
        t = e->death_texture;
        e->frame_count = 6;
        e->frame_delay = 120;
        break;

    /* New mirrored states */
    case ENEMY_SLIDE:
        t = e->slide_texture;
        e->frame_count = 6;
        e->frame_delay = 80;
        break;
    case ENEMY_BLOCK_HURT:
        t = e->block_hurt_texture;
        e->frame_count = 3;
        e->frame_delay = 100;
        e->block_hurt_start_time = SDL_GetTicks();
        break;
    case ENEMY_PRAY:
        t = e->pray_texture;
        e->frame_count = 6;
        e->frame_delay = 120;
        break;
    case ENEMY_DOWN_ATTACK:
        t = e->down_attack_texture;
        e->frame_count = 6;
        e->frame_delay = 80;
        break;
    }

    if (t)
    {
        tex_dims(t, &tw, &th);
        e->frame_width = (e->frame_count > 0) ? (float)tw / (float)e->frame_count : (float)tw;
        e->src_rect.w = (int)e->frame_width;
        e->dest_rect.w = (int)e->frame_width;
    }
    e->src_rect.x = 0;
}

void update_enemy(Enemy *e, Uint32 dt_ms)
{
    if (!e)
        return;

    float dt = dt_ms / 1000.0f;

    /* integrate motion */
    e->x += e->velocity_x * dt;
    e->y += e->velocity_y * dt;

    /* gravity */
    if (!e->on_ground)
        e->velocity_y += e->gravity * dt;

    /* ground resolve */
    if (e->y >= GROUND_LEVEL)
    {
        e->y = GROUND_LEVEL;
        e->velocity_y = 0;
        if (!e->on_ground)
        {
            e->on_ground = 1;
            /* land from air-only states */
            if (e->state == ENEMY_JUMPING || e->state == ENEMY_DOWN_ATTACK)
                set_enemy_state(e, ENEMY_IDLE);
        }
    }
    else
    {
        e->on_ground = 0;
    }

    /* attack timer (mirror player timing exit) */
    if (e->is_attacking)
    {
        Uint32 now = SDL_GetTicks();
        if (now - e->attack_start_time >= e->attack_duration)
        {
            e->is_attacking = 0;
            set_enemy_state(e, e->on_ground ? ENEMY_IDLE : ENEMY_JUMPING);
        }
    }

    /* block-hurt timer */
    if (e->state == ENEMY_BLOCK_HURT)
    {
        if (SDL_GetTicks() - e->block_hurt_start_time >= e->block_hurt_duration)
        {
            set_enemy_state(e, e->on_ground ? ENEMY_IDLE : ENEMY_JUMPING);
        }
    }

    /* advance animation frames */
    Uint32 now = SDL_GetTicks();
    if (now - e->last_frame_time >= (Uint32)e->frame_delay)
    {
        if (e->state == ENEMY_DEATH || e->state == ENEMY_PRAY)
        {
            if (e->current_frame < e->frame_count - 1)
            {
                e->current_frame++;
                e->src_rect.x = e->current_frame * (int)e->frame_width;
            }
            /* Optional: auto-return from PRAY after one full playthrough */
            if (e->state == ENEMY_PRAY && e->current_frame >= e->frame_count - 1)
            {
                set_enemy_state(e, ENEMY_IDLE);
            }
        }
        else if (e->state == ENEMY_ATTACKING || e->state == ENEMY_DOWN_ATTACK)
        {
            if (e->current_frame < e->frame_count - 1)
                e->current_frame++;
            e->src_rect.x = e->current_frame * (int)e->frame_width;
        }
        else
        {
            e->current_frame = (e->current_frame + 1) % e->frame_count;
            e->src_rect.x = e->current_frame * (int)e->frame_width;
        }
        e->last_frame_time = now;
    }

    /* sync rects */
    e->dest_rect.x = (int)e->x;
    e->dest_rect.y = (int)e->y;
    e->dest_rect.h = ENEMY_HEIGHT;

    /* simple horizontal clamp (match player limits if you have them) */
    if (e->x < -200)
        e->x = -200;
    if (e->x > 1280 + 200 - e->frame_width)
        e->x = 1280 + 200 - e->frame_width;
}

void render_enemy(SDL_Renderer *renderer, Enemy *e)
{
    if (!renderer || !e)
        return;

    SDL_Texture *tex = NULL;
    switch (e->state)
    {
    case ENEMY_IDLE:
        tex = e->idle_texture;
        break;
    case ENEMY_WALKING:
        tex = e->walking_texture;
        break;
    case ENEMY_REPOSITIONING:
        tex = e->reposition_texture;
        break;
    case ENEMY_JUMPING:
        tex = e->jumping_texture;
        break;
    case ENEMY_ATTACKING:
        tex = (e->current_attack == 0 ? e->attack_texture : (e->current_attack == 1 ? e->attack2_texture : e->attack3_texture));
        break;
    case ENEMY_BLOCKING:
        tex = e->block_texture;
        break;
    case ENEMY_HURT:
        tex = e->hurt_texture;
        break;
    case ENEMY_DEATH:
        tex = e->death_texture;
        break;
    case ENEMY_SLIDE:
        tex = e->slide_texture;
        break;
    case ENEMY_BLOCK_HURT:
        tex = e->block_hurt_texture;
        break;
    case ENEMY_PRAY:
        tex = e->pray_texture;
        break;
    case ENEMY_DOWN_ATTACK:
        tex = e->down_attack_texture;
        break;
    }
    if (!tex)
        return;

    SDL_RendererFlip flip = (e->direction == L) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, tex, &e->src_rect, &e->dest_rect, 0, NULL, flip);
}
