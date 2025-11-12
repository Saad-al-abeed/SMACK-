#include "player2.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define GROUND_LEVEL 375
#define PLAYER2_HEIGHT 258
#define BLOCK_HURT_DURATION_MS 500

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *s = SDL_LoadBMP(path);
    if (!s)
    {
        fprintf(stderr, "P2: load %s failed: %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, 255, 0, 255));
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    if (!t)
        fprintf(stderr, "P2: create tex %s failed: %s\n", path, SDL_GetError());
    return t;
}

static void tex_dims(SDL_Texture *t, int *w, int *h) { SDL_QueryTexture(t, NULL, NULL, w, h); }

Player2 *create_player2(SDL_Renderer *renderer, float x, float y)
{
    Player2 *p = (Player2 *)malloc(sizeof(Player2));
    if (!p)
        return NULL;

    p->x = x;
    p->y = y;
    p->velocity_x = 0;
    p->velocity_y = 0;
    p->speed = 450.0f;
    p->jump_force = -900.0f;
    p->gravity = 1500.0f;
    p->on_ground = false;

    p->idle_texture = load_texture(renderer, "assets/textures/Final/Idle_h258_w516.bmp");
    p->walking_texture = load_texture(renderer, "assets/textures/Final/Run_h258_w516.bmp");
    p->jumping_texture = load_texture(renderer, "assets/textures/Final/nor_jmp_h258_w516.bmp");
    p->attack_texture = load_texture(renderer, "assets/textures/Final/atk1.bmp");
    p->attack2_texture = load_texture(renderer, "assets/textures/Final/atk3.bmp");
    p->attack3_texture = load_texture(renderer, "assets/textures/Final/atk4.bmp");
    p->block_texture = load_texture(renderer, "assets/textures/Final/crouch_idle-sheet.bmp");
    p->hurt_texture = load_texture(renderer, "assets/textures/Final/Hurt-sheet.bmp"); // Add this
    p->death_texture = load_texture(renderer, "assets/textures/Final/Dth_h258_w516.bmp");
    p->slide_texture = load_texture(renderer, "assets/textures/Final/Slide-sheet.bmp");
    p->block_hurt_texture = load_texture(renderer, "assets/textures/Final/blockhurt.bmp");
    p->pray_texture = load_texture(renderer, "assets/textures/Final/pray_h258_w516.bmp");
    p->down_attack_texture = load_texture(renderer, "assets/textures/jmph258w516.bmp");

    if (!p->idle_texture || !p->walking_texture || !p->jumping_texture ||
        !p->attack_texture || !p->attack2_texture || !p->attack3_texture ||
        !p->block_texture || !p->hurt_texture || !p->death_texture ||
        !p->slide_texture || !p->block_hurt_texture ||
        !p->pray_texture || !p->down_attack_texture)
    {
        fprintf(stderr, "P2: placeholder textures missing.\n");
        destroy_player2(p);
        return NULL;
    }

    p->current_frame = 0;
    p->frame_height = PLAYER2_HEIGHT;
    p->last_frame_time = SDL_GetTicks();
    int tw, th;
    tex_dims(p->idle_texture, &tw, &th);
    p->frame_width = (float)tw;
    p->frame_count = 1;
    p->frame_delay = 150;

    p->src_rect = (SDL_Rect){0, 0, (int)p->frame_width, p->frame_height};
    p->dest_rect = (SDL_Rect){(int)x, (int)y, (int)p->frame_width, p->frame_height};

    p->state = PLAYER2_IDLE;
    p->direction = LEFT;
    p->is_attacking = false;
    p->is_blocking = false;
    p->attack_start_time = 0;
    p->attack_duration = 500;
    p->current_attack = 0;

    p->block_hurt_start_time = 0;
    p->block_hurt_duration = BLOCK_HURT_DURATION_MS;

    set_player2_state(p, PLAYER2_IDLE);
    return p;
}

void destroy_player2(Player2 *p)
{
    if (!p)
        return;
    SDL_Texture **txs[] = {
        &p->idle_texture, &p->walking_texture, &p->jumping_texture,
        &p->attack_texture, &p->attack2_texture, &p->attack3_texture,
        &p->block_texture, &p->hurt_texture, &p->death_texture,
        &p->slide_texture, &p->block_hurt_texture, &p->pray_texture, &p->down_attack_texture};
    for (size_t i = 0; i < sizeof(txs) / sizeof(txs[0]); ++i)
        if (*txs[i])
            SDL_DestroyTexture(*txs[i]);
    free(p);
}

void handle_player2_input(Player2 *p, const Uint8 *keystate)
{
    if (!p || p->state == PLAYER2_DEATH)
        return;
    if (p->state == PLAYER2_HURT)
        return;

    p->velocity_x = 0.0f;

    bool busy_attack = p->is_attacking &&
                       (p->state == PLAYER2_ATTACKING || p->state == PLAYER2_DOWN_ATTACK);
    bool busy_block = p->is_blocking || p->state == PLAYER2_BLOCKING;

    /* Movement keys */
    bool leftHeld = keystate[SDL_SCANCODE_LEFT];
    bool rightHeld = keystate[SDL_SCANCODE_RIGHT];
    bool movingHeld = leftHeld || rightHeld;

    /* Horizontal movement */
    if (leftHeld && !busy_attack && !busy_block)
    {
        p->velocity_x = -p->speed;
        p->direction = LEFT;
        if (p->on_ground && p->state != PLAYER2_SLIDE)
            set_player2_state(p, PLAYER2_WALKING);
    }
    else if (rightHeld && !busy_attack && !busy_block)
    {
        p->velocity_x = p->speed;
        p->direction = RIGHT;
        if (p->on_ground && p->state != PLAYER2_SLIDE)
            set_player2_state(p, PLAYER2_WALKING);
    }
    else
    {
        if (p->state == PLAYER2_WALKING && p->on_ground)
            set_player2_state(p, PLAYER2_IDLE);
    }

    /* Jump (KP_ENTER) — unchanged */
    if (keystate[SDL_SCANCODE_KP_ENTER] && p->on_ground && !busy_attack && !busy_block)
    {
        p->velocity_y = p->jump_force;
        p->on_ground = false;
        set_player2_state(p, PLAYER2_JUMPING);
    }

    /* ===== If moving (Left/Right) is held, ignore UP (attack) and DOWN (block) ===== */

    /* UP: attack (works on ground and in air), only when NOT moving */
    static bool up_pressed = false;
    if (!movingHeld && keystate[SDL_SCANCODE_UP] && !busy_attack && !busy_block)
    {
        if (!up_pressed)
        {
            p->is_attacking = true;
            p->attack_start_time = SDL_GetTicks();
            p->current_attack = (p->current_attack + 1) % 3;
            set_player2_state(p, PLAYER2_ATTACKING); /* shows attack even in air */
            up_pressed = true;
        }
    }
    else
    {
        up_pressed = false; /* also clears while moving so no stale press fires */
    }

    /* Aerial straight-down attack (KP_0) — unchanged */
    static bool kp0_pressed = false;
    if (keystate[SDL_SCANCODE_KP_0] && !busy_attack && !busy_block && !p->on_ground)
    {
        if (!kp0_pressed)
        {
            p->is_attacking = true;
            p->attack_start_time = SDL_GetTicks();
            set_player2_state(p, PLAYER2_DOWN_ATTACK);
            if (p->velocity_y < 600.0f)
                p->velocity_y = 600.0f;
            kp0_pressed = true;
        }
    }
    else
    {
        kp0_pressed = false;
    }

    /* DOWN: block while held — only when NOT moving; works in air as well */
    if (!movingHeld && keystate[SDL_SCANCODE_DOWN] && !busy_attack)
    {
        if (!p->is_blocking)
        {
            p->is_blocking = true;
            set_player2_state(p, PLAYER2_BLOCKING); /* shows block even in air */
        }
    }
    else
    {
        if (p->is_blocking)
        {
            p->is_blocking = false;
            /* return to idle if grounded, jump anim if airborne */
            set_player2_state(p, p->on_ground ? PLAYER2_IDLE : PLAYER2_JUMPING);
        }
    }

    /* Slide on Numpad '+' (ground only) — unchanged */
    bool kp_plus = keystate[SDL_SCANCODE_KP_PLUS];
    if (kp_plus && p->on_ground && !busy_attack && !busy_block)
    {
        if (p->state != PLAYER2_SLIDE)
            set_player2_state(p, PLAYER2_SLIDE);
        p->velocity_x = (p->direction == RIGHT ? p->speed * 1.2f : -p->speed * 1.2f);
    }
    else
    {
        if (p->state == PLAYER2_SLIDE && p->on_ground)
            set_player2_state(p, PLAYER2_IDLE);
    }

    /* Keep jump anim while airborne, but DON'T override attack/block/down-attack/hurt */
    if (!p->on_ground &&
        p->state != PLAYER2_JUMPING &&
        p->state != PLAYER2_DOWN_ATTACK &&
        p->state != PLAYER2_HURT &&
        p->state != PLAYER2_ATTACKING &&
        p->state != PLAYER2_BLOCKING)
    {
        set_player2_state(p, PLAYER2_JUMPING);
    }
}

void update_player2(Player2 *p, Uint32 dt_ms)
{
    if (!p)
        return;
    float dt = dt_ms / 1000.0f;

    p->x += p->velocity_x * dt;
    p->y += p->velocity_y * dt;

    if (!p->on_ground)
        p->velocity_y += p->gravity * dt;

    if (p->y >= GROUND_LEVEL)
    {
        p->y = GROUND_LEVEL;
        p->velocity_y = 0;
        if (!p->on_ground)
        {
            p->on_ground = true;
            if (p->state == PLAYER2_JUMPING || p->state == PLAYER2_DOWN_ATTACK)
                set_player2_state(p, PLAYER2_IDLE);
        }
    }
    else
        p->on_ground = false;

    if (p->is_attacking)
    {
        Uint32 now = SDL_GetTicks();
        if (now - p->attack_start_time >= p->attack_duration)
        {
            p->is_attacking = false;
            // MODIFIED: Only change state if the player is still in an attack state.
            // This prevents overwriting the DEATH state.
            if (p->state == PLAYER2_ATTACKING || p->state == PLAYER2_DOWN_ATTACK)
            {
                if (p->on_ground)
                    set_player2_state(p, PLAYER2_IDLE);
                else
                    set_player2_state(p, PLAYER2_JUMPING);
            }
        }
    }

    if (p->state == PLAYER2_BLOCK_HURT)
    {
        if (SDL_GetTicks() - p->block_hurt_start_time >= p->block_hurt_duration)
            set_player2_state(p, p->on_ground ? PLAYER2_IDLE : PLAYER2_JUMPING);
    }

    Uint32 now = SDL_GetTicks();
    if (now - p->last_frame_time >= p->frame_delay)
    {
        if (p->state == PLAYER2_DEATH || p->state == PLAYER2_PRAY)
        {
            if (p->current_frame < p->frame_count - 1)
            {
                p->current_frame++;
                p->src_rect.x = p->current_frame * p->frame_width;
            }
            if (p->state == PLAYER2_PRAY && p->current_frame >= p->frame_count - 1)
                set_player2_state(p, PLAYER2_IDLE);
        }
        else if (p->state == PLAYER2_ATTACKING || p->state == PLAYER2_DOWN_ATTACK)
        {
            if (p->current_frame < p->frame_count - 1)
                p->current_frame++;
            p->src_rect.x = p->current_frame * p->frame_width;
        }
        else
        {
            p->current_frame = (p->current_frame + 1) % p->frame_count;
            p->src_rect.x = p->current_frame * p->frame_width;
        }
        p->last_frame_time = now;
    }

    p->dest_rect.x = (int)p->x;
    p->dest_rect.y = (int)p->y;
    p->dest_rect.w = (int)p->frame_width;
    p->dest_rect.h = PLAYER2_HEIGHT;

    if (p->x < -200)
        p->x = -200;
    if (p->x > 1280 + 200 - p->frame_width)
        p->x = 1280 + 200 - p->frame_width;
}

void render_player2(SDL_Renderer *r, Player2 *p)
{
    if (!r || !p)
        return;
    SDL_Texture *t = NULL;

    switch (p->state)
    {
    case PLAYER2_IDLE:
        t = p->idle_texture;
        break;
    case PLAYER2_WALKING:
        t = p->walking_texture;
        break;
    case PLAYER2_JUMPING:
        t = p->jumping_texture;
        break;
    case PLAYER2_ATTACKING:
        t = (p->current_attack == 0 ? p->attack_texture : (p->current_attack == 1 ? p->attack2_texture : p->attack3_texture));
        break;
    case PLAYER2_BLOCKING:
        t = p->block_texture;
        break;
    case PLAYER2_HURT:
        t = p->hurt_texture;
        break;
    case PLAYER2_DEATH:
        t = p->death_texture;
        break;
    /* NEW */
    case PLAYER2_SLIDE:
        t = p->slide_texture;
        break;
    case PLAYER2_BLOCK_HURT:
        t = p->block_hurt_texture;
        break;
    case PLAYER2_PRAY:
        t = p->pray_texture;
        break;
    case PLAYER2_DOWN_ATTACK:
        t = p->down_attack_texture;
        break;
    }

    SDL_RendererFlip flip = (p->direction == LEFT ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    if (t)
        SDL_RenderCopyEx(r, t, &p->src_rect, &p->dest_rect, 0, NULL, flip);
}

void set_player2_state(Player2 *p, Player2State s)
{
    if (!p)
        return;
    if (p->state == s && s != PLAYER2_ATTACKING && s != PLAYER2_DOWN_ATTACK)
        return;

    p->state = s;
    p->current_frame = 0;
    p->last_frame_time = SDL_GetTicks();
    SDL_Texture *t = NULL;
    int tw = 0, th = 0;

    switch (s)
    {
    case PLAYER2_IDLE:
        t = p->idle_texture;
        p->frame_count = 8;
        p->frame_delay = 100;
        break;
    case PLAYER2_WALKING:
        t = p->walking_texture;
        p->frame_count = 8;
        p->frame_delay = 100;
        break;
    case PLAYER2_JUMPING:
        t = p->jumping_texture;
        p->frame_count = 8;
        p->frame_delay = 100;
        break;
    case PLAYER2_BLOCKING:
        t = p->block_texture;
        p->frame_count = 1;
        p->frame_delay = 100;
        break;
    case PLAYER2_DEATH:
        t = p->death_texture;
        p->frame_count = 4;
        p->frame_delay = 125;
        break;
    case PLAYER2_HURT:
        t = p->hurt_texture;
        p->frame_count = 3;
        p->frame_delay = 100;
        break;
    case PLAYER2_ATTACKING:
        switch (p->current_attack)
        {
        case 0:
            t = p->attack_texture;
            p->frame_count = 6; // Attack 1 has 5 frames
            p->frame_delay = 100;
            break;
        case 1:
            t = p->attack2_texture;
            p->frame_count = 4; // Attack 2 has 4 frames
            p->frame_delay = 150;
            break;
        case 2:
            t = p->attack3_texture;
            p->frame_count = 6; // Attack 3 has 4 frames
            p->frame_delay = 100;
            break;
        }
        break;
    /* NEW */
    case PLAYER2_SLIDE:
        t = p->slide_texture;
        p->frame_count = 10;
        p->frame_delay = 80;
        break;
    case PLAYER2_BLOCK_HURT:
        t = p->block_hurt_texture;
        p->frame_count = 6;
        p->frame_delay = 83;
        p->block_hurt_start_time = SDL_GetTicks();
        break;
    case PLAYER2_PRAY:
        t = p->pray_texture;
        p->frame_count = 12;
        p->frame_delay = 100;
        break;
    case PLAYER2_DOWN_ATTACK:
        t = p->down_attack_texture;
        p->frame_count = 7;
        p->frame_delay = 140;
        break;
    }

    if (t)
    {
        tex_dims(t, &tw, &th);
        p->frame_width = (float)tw / (float)p->frame_count;
        p->src_rect.w = (int)p->frame_width;
        p->dest_rect.w = (int)p->frame_width;
    }
    p->src_rect.x = 0;
}