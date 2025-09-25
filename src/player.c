#include "player.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

extern void sound__p_idle(void);
extern void sound__p_walk(void);
extern void sound__p_jump(void);
extern void sound__p_atk(int);
extern void sound__p_block(void);
extern void sound__p_hurt(void);
extern void sound__p_death(void);
extern void sound__p_slide(void);
extern void sound__p_block_hurt(void);
extern void sound__p_pray(void);
extern void sound__p_down_attack(void);


#define GROUND_LEVEL 375
#define PLAYER_HEIGHT 258

/* PLACEHOLDER tunables */
#define BLOCK_HURT_DURATION_MS 300

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface)
    {
        fprintf(stderr, "Failed to load BMP %s: %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
        fprintf(stderr, "Failed to create texture from %s: %s\n", path, SDL_GetError());
    return texture;
}

static void get_texture_dimensions(SDL_Texture *texture, int *w, int *h)
{
    SDL_QueryTexture(texture, NULL, NULL, w, h);
}

Player *create_player(SDL_Renderer *renderer, float x, float y)
{
    Player *player = (Player *)malloc(sizeof(Player));
    if (!player)
        return NULL;

    /* Physics */
    player->x = x;
    player->y = y;
    player->velocity_x = 0;
    player->velocity_y = 0;
    player->speed = 450.0f;
    player->jump_force = -900.0f;
    player->gravity = 1500.0f;
    player->on_ground = false;

    /* Textures init */
    player->idle_texture = NULL;
    player->walking_texture = NULL;
    player->jumping_texture = NULL;
    player->attack_texture = NULL;
    player->attack2_texture = NULL;
    player->attack3_texture = NULL;
    player->block_texture = NULL;
    player->hurt_texture = NULL;
    player->death_texture = NULL;

    /* NEW */
    player->slide_texture = NULL;
    player->block_hurt_texture = NULL;
    player->pray_texture = NULL;
    player->down_attack_texture = NULL;

    /* Load (PLACEHOLDER paths) */
    player->idle_texture = load_texture(renderer, "assets/textures/Final/Idle_h258_w516.bmp");
    player->walking_texture = load_texture(renderer, "assets/textures/Final/Run_h258_w516.bmp");
    player->jumping_texture = load_texture(renderer, "assets/textures/Final/nor_jmp_h258_w516.bmp");
    player->attack_texture = load_texture(renderer, "assets/textures/Final/atk1.bmp");
    player->attack2_texture = load_texture(renderer, "assets/textures/Final/atk3.bmp");
    player->attack3_texture = load_texture(renderer, "assets/textures/Final/atk4.bmp");
    player->block_texture = load_texture(renderer, "assets/textures/Final/crouch_idle-sheet.bmp");
    player->hurt_texture = load_texture(renderer, "assets/textures/Final/Hurt-sheet.bmp"); // Add this
    player->death_texture = load_texture(renderer, "assets/textures/Final/Dth_h258_w516.bmp");
    player->slide_texture = load_texture(renderer, "assets/textures/Final/Slide-sheet.bmp");
    player->block_hurt_texture = load_texture(renderer, "assets/textures/Final/blockhurt.bmp");
    player->pray_texture = load_texture(renderer, "assets/textures/Final/pray_h258_w516.bmp");
    player->down_attack_texture = load_texture(renderer, "assets/textures/Final/jmph258w516.bmp");

    /* Basic sanity (you will replace with real assets) */
    if (!player->idle_texture || !player->walking_texture || !player->jumping_texture ||
        !player->attack_texture || !player->attack2_texture || !player->attack3_texture ||
        !player->block_texture || !player->hurt_texture || !player->death_texture ||
        !player->slide_texture || !player->block_hurt_texture ||
        !player->pray_texture || !player->down_attack_texture)
    {
        fprintf(stderr, "Player: one or more textures failed to load (placeholders).\n");
        destroy_player(player);
        return NULL;
    }

    /* Animation init */
    player->current_frame = 0;
    player->frame_height = PLAYER_HEIGHT;
    player->last_frame_time = SDL_GetTicks();

    int tw, th;
    get_texture_dimensions(player->idle_texture, &tw, &th);
    player->frame_width = (float)tw; /* idle default; will be updated by state */
    player->frame_count = 1;
    player->frame_delay = 150;

    /* Rects */
    player->src_rect = (SDL_Rect){0, 0, (int)player->frame_width, player->frame_height};
    player->dest_rect = (SDL_Rect){(int)x, (int)y, (int)player->frame_width, player->frame_height};

    /* State & combat */
    player->state = PLAYER_IDLE;
    player->direction = FACING_RIGHT;
    player->is_attacking = false;
    player->is_blocking = false;
    player->attack_start_time = 0;
    player->attack_duration = 500; /* placeholder */
    player->current_attack = 0;

    /* NEW: block-hurt timer */
    player->block_hurt_start_time = 0;
    player->block_hurt_duration = BLOCK_HURT_DURATION_MS;

    set_player_state(player, PLAYER_IDLE);
    return player;
}

void destroy_player(Player *player)
{
    if (!player)
        return;
    SDL_Texture **txs[] = {
        &player->idle_texture, &player->walking_texture, &player->jumping_texture,
        &player->attack_texture, &player->attack2_texture, &player->attack3_texture,
        &player->block_texture, &player->hurt_texture, &player->death_texture,
        &player->slide_texture, &player->block_hurt_texture, &player->pray_texture,
        &player->down_attack_texture};
    for (size_t i = 0; i < sizeof(txs) / sizeof(txs[0]); ++i)
        if (*txs[i])
            SDL_DestroyTexture(*txs[i]);
    free(player);
}

void handle_player_input(Player *player, const Uint8 *keystate)
{
    if (!player || player->state == PLAYER_DEATH)
        return;

    /* While truly hurt, ignore input */
    if (player->state == PLAYER_HURT)
        return;

    /* Reset horizontal velocity each frame; states will set as needed */
    player->velocity_x = 0.0f;

    /* Busy flags (unchanged) */
    bool busy_attack = player->is_attacking &&
                       (player->state == PLAYER_ATTACKING || player->state == PLAYER_DOWN_ATTACK);
    bool busy_block = player->is_blocking || player->state == PLAYER_BLOCKING;

    /* Movement keys */
    bool leftHeld = keystate[SDL_SCANCODE_A];
    bool rightHeld = keystate[SDL_SCANCODE_D];
    bool movingHeld = leftHeld || rightHeld;

    /* Horizontal movement (unchanged) */
    if (leftHeld && !busy_attack && !busy_block)
    {
        player->velocity_x = -player->speed;
        player->direction = FACING_LEFT;
        if (player->on_ground && player->state != PLAYER_SLIDE)
            set_player_state(player, PLAYER_WALKING);
    }
    else if (rightHeld && !busy_attack && !busy_block)
    {
        player->velocity_x = player->speed;
        player->direction = FACING_RIGHT;
        if (player->on_ground && player->state != PLAYER_SLIDE)
            set_player_state(player, PLAYER_WALKING);
    }
    else
    {
        if (player->state == PLAYER_WALKING && player->on_ground)
            set_player_state(player, PLAYER_IDLE);
    }

    /* Jump (SPACE) unchanged */
    if (keystate[SDL_SCANCODE_SPACE] && player->on_ground && !busy_attack && !busy_block)
    {
        player->velocity_y = player->jump_force;
        player->on_ground = false;
        set_player_state(player, PLAYER_JUMPING);
    }

    /* ===== Gating rule: if A/D is held, ignore W (attack) and S (block) ===== */

    /* W: attack (now allowed both on ground and in air), but only if NOT moving */
    static bool w_was_pressed = false;
    if (!movingHeld && keystate[SDL_SCANCODE_W] && !busy_attack && !busy_block)
    {
        if (!w_was_pressed)
        {
            player->is_attacking = true;
            player->attack_start_time = SDL_GetTicks();
            player->current_attack = (player->current_attack + 1) % 3;
            set_player_state(player, PLAYER_ATTACKING); /* will show attack even in air */
            w_was_pressed = true;
        }
    }
    else
    {
        w_was_pressed = false; /* clear edge flag (also clears while moving) */
    }

    /* Aerial straight-down attack with F (unchanged) */
    static bool f_was_pressed = false;
    if (keystate[SDL_SCANCODE_F] && !busy_attack && !busy_block && !player->on_ground)
    {
        if (!f_was_pressed)
        {
            player->is_attacking = true;
            player->attack_start_time = SDL_GetTicks();
            set_player_state(player, PLAYER_DOWN_ATTACK);
            if (player->velocity_y < 600.0f)
                player->velocity_y = 600.0f; /* give it some oomph */
            f_was_pressed = true;
        }
    }
    else
    {
        f_was_pressed = false;
    }

    /* S: block while held — only when NOT moving; works in air as well */
    if (!movingHeld && keystate[SDL_SCANCODE_S] && !busy_attack)
    {
        if (!player->is_blocking)
        {
            player->is_blocking = true;
            set_player_state(player, PLAYER_BLOCKING); /* shows block even in air */
        }
    }
    else
    {
        if (player->is_blocking)
        {
            player->is_blocking = false;
            /* return to idle if grounded, jump anim if airborne */
            set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
        }
    }

    /* Slide on ALT (ground only) – unchanged */
    bool alt_held = keystate[SDL_SCANCODE_LALT] || keystate[SDL_SCANCODE_RALT];
    if (alt_held && player->on_ground && !busy_attack && !busy_block)
    {
        if (player->state != PLAYER_SLIDE)
            set_player_state(player, PLAYER_SLIDE);
        player->velocity_x = (player->direction == FACING_RIGHT ? player->speed * 1.2f : -player->speed * 1.2f);
    }
    else
    {
        if (player->state == PLAYER_SLIDE && player->on_ground)
            set_player_state(player, PLAYER_IDLE);
    }

    /* Keep "in-air" anim fresh, but DON'T override aerial attack/block/down-attack/hurt */
    if (!player->on_ground &&
        player->state != PLAYER_JUMPING &&
        player->state != PLAYER_DOWN_ATTACK &&
        player->state != PLAYER_HURT &&
        player->state != PLAYER_ATTACKING &&
        player->state != PLAYER_BLOCKING)
    {
        set_player_state(player, PLAYER_JUMPING);
    }
}

void update_player(Player *player, Uint32 delta_time)
{
    if (!player)
        return;

    float dt = delta_time / 1000.0f;

    /* Integrate */
    player->x += player->velocity_x * dt;
    player->y += player->velocity_y * dt;

    /* Gravity */
    if (!player->on_ground)
        player->velocity_y += player->gravity * dt;

    /* Ground */
    if (player->y >= GROUND_LEVEL)
    {
        player->y = GROUND_LEVEL;
        player->velocity_y = 0;
        if (!player->on_ground)
        {
            player->on_ground = true;
            if (player->state == PLAYER_JUMPING || player->state == PLAYER_DOWN_ATTACK)
                set_player_state(player, PLAYER_IDLE);
        }
    }
    else
        player->on_ground = false;

    /* Attack timers */
    if (player->is_attacking)
    {
        Uint32 now = SDL_GetTicks();
        if (now - player->attack_start_time >= player->attack_duration)
        {
            player->is_attacking = false;
            if (player->on_ground)
                set_player_state(player, PLAYER_IDLE);
            else
                set_player_state(player, PLAYER_JUMPING);
        }
    }

    /* Block-hurt timer (independent of real hurt) */
    if (player->state == PLAYER_BLOCK_HURT)
    {
        if (SDL_GetTicks() - player->block_hurt_start_time >= player->block_hurt_duration)
            set_player_state(player, player->on_ground ? PLAYER_IDLE : PLAYER_JUMPING);
    }

    /* Animation advance */
    Uint32 now = SDL_GetTicks();
    if (now - player->last_frame_time >= player->frame_delay)
    {
        if (player->state == PLAYER_DEATH || player->state == PLAYER_PRAY)
        {
            if (player->current_frame < player->frame_count - 1)
            {
                player->current_frame++;
                player->src_rect.x = player->current_frame * player->frame_width;
            }
            /* freeze on last death frame; for PRAY weâ€™ll drop to idle below */
            if (player->state == PLAYER_PRAY && player->current_frame >= player->frame_count - 1)
                set_player_state(player, PLAYER_IDLE);
        }
        else if (player->state == PLAYER_ATTACKING || player->state == PLAYER_DOWN_ATTACK)
        {
            if (player->current_frame < player->frame_count - 1)
                player->current_frame++;
            player->src_rect.x = player->current_frame * player->frame_width;
        }
        else
        {
            player->current_frame = (player->current_frame + 1) % player->frame_count;
            player->src_rect.x = player->current_frame * player->frame_width;
        }
        player->last_frame_time = now;
    }

    /* Dest rect */
    player->dest_rect.x = (int)player->x;
    player->dest_rect.y = (int)player->y;
    player->dest_rect.w = (int)player->frame_width;
    player->dest_rect.h = player->frame_height;

    /* Screen clamp (simple) */
    if (player->x < -200)
        player->x = -200;
    if (player->x > 1280 + 200 - player->frame_width)
        player->x = 1280 + 200 - player->frame_width;
}

void render_player(SDL_Renderer *renderer, Player *player)
{
    if (!player || !renderer)
        return;
    SDL_Texture *tex = NULL;

    switch (player->state)
    {
    case PLAYER_IDLE:
        tex = player->idle_texture;
        break;
    case PLAYER_WALKING:
        tex = player->walking_texture;
        break;
    case PLAYER_JUMPING:
        tex = player->jumping_texture;
        break;
    case PLAYER_ATTACKING:
        switch (player->current_attack)
        {
        case 0:
            tex = player->attack_texture;
            break;
        case 1:
            tex = player->attack2_texture;
            break;
        default:
            tex = player->attack3_texture;
            break;
        }
        break;
    case PLAYER_BLOCKING:
        tex = player->block_texture;
        break;
    case PLAYER_HURT:
        tex = player->hurt_texture;
        break;
    case PLAYER_DEATH:
        tex = player->death_texture;
        break;
    /* NEW */
    case PLAYER_SLIDE:
        tex = player->slide_texture;
        break;
    case PLAYER_BLOCK_HURT:
        tex = player->block_hurt_texture;
        break;
    case PLAYER_PRAY:
        tex = player->pray_texture;
        break;
    case PLAYER_DOWN_ATTACK:
        tex = player->down_attack_texture;
        break;
    }

    if (!tex)
        return;

    SDL_RendererFlip flip = (player->direction == FACING_LEFT) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, tex, &player->src_rect, &player->dest_rect, 0, NULL, flip);
}

void set_player_state(Player *player, PlayerState s)
{
    if (!player)
        return;
    if (player->state == s && s != PLAYER_ATTACKING && s != PLAYER_DOWN_ATTACK)
        return;

    player->state = s;
    player->current_frame = 0;
    player->last_frame_time = SDL_GetTicks();

    SDL_Texture *t = NULL;
    int tw = 0, th = 0;

    switch (s)
    {
    case PLAYER_IDLE:
        t = player->idle_texture;
        player->frame_count = 8;
        player->frame_delay = 100;
        break;
    case PLAYER_WALKING:
        t = player->walking_texture;
        player->frame_count = 8;
        player->frame_delay = 100;
        break;
    case PLAYER_JUMPING:
        t = player->jumping_texture;
        player->frame_count = 8;
        player->frame_delay = 100;
        break;
    case PLAYER_BLOCKING:
        t = player->block_texture;
        player->frame_count = 1;
        player->frame_delay = 100;
        break;
    case PLAYER_DEATH:
        t = player->death_texture;
        player->frame_count = 4;
        player->frame_delay = 125;
        break;
    case PLAYER_HURT:
        t = player->hurt_texture;
        player->frame_count = 3;
        player->frame_delay = 100;
        break;
    case PLAYER_ATTACKING:
        switch (player->current_attack)
        {
        case 0:
            t = player->attack_texture;
            player->frame_count = 6; // Attack 1 has 5 frames
            player->frame_delay = 100;
            break;
        case 1:
            t = player->attack2_texture;
            player->frame_count = 4; // Attack 2 has 4 frames
            player->frame_delay = 150;
            break;
        case 2:
            t = player->attack3_texture;
            player->frame_count = 6; // Attack 3 has 4 frames
            player->frame_delay = 100;
            break;
        }
        break;
    /* NEW */
    case PLAYER_SLIDE:
        t = player->slide_texture;
        player->frame_count = 10;
        player->frame_delay = 80;
        break;
    case PLAYER_BLOCK_HURT:
        t = player->block_hurt_texture;
        player->frame_count = 6;
        player->frame_delay = 50;
        player->block_hurt_start_time = SDL_GetTicks();
        break;
    case PLAYER_PRAY:
        t = player->pray_texture;
        player->frame_count = 12;
        player->frame_delay = 100;
        break;
    case PLAYER_DOWN_ATTACK:
        t = player->down_attack_texture;
        player->frame_count = 7;
        player->frame_delay = 140;
        break;
    }

    if (t)
    {
        get_texture_dimensions(t, &tw, &th);
        player->frame_width = (float)tw / (float)player->frame_count;
        player->src_rect.w = (int)player->frame_width;
        player->dest_rect.w = (int)player->frame_width;
    }
    player->src_rect.x = 0;

    /* === sound hook === */
    switch (player->state) {
        case PLAYER_IDLE:        sound__p_idle(); break;
        case PLAYER_WALKING:     sound__p_walk(); break;
        case PLAYER_JUMPING:     sound__p_jump(); break;
        case PLAYER_ATTACKING:   sound__p_atk(player->current_attack); break;
        case PLAYER_BLOCKING:    sound__p_block(); break;
        case PLAYER_HURT:        sound__p_hurt(); break;
        case PLAYER_DEATH:       sound__p_death(); break;
        case PLAYER_SLIDE:       sound__p_slide(); break;
        case PLAYER_BLOCK_HURT:  sound__p_block_hurt(); break;
        case PLAYER_PRAY:        sound__p_pray(); break;
        case PLAYER_DOWN_ATTACK: sound__p_down_attack(); break;
        default: break;
    }
}