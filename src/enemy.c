#include "enemy.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define GROUND_LEVEL 375 // Adjust based on your map
#define ENEMY_HEIGHT 258

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface)
    {
        fprintf(stderr, "Failed to load BMP %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    // Set color key for transparency (assuming magenta is transparent)
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        fprintf(stderr, "Failed to create texture from %s: %s\n", path, SDL_GetError());
    }

    return texture;
}

// Helper function to get texture dimensions
static void get_texture_dimensions(SDL_Texture *texture, int *w, int *h)
{
    SDL_QueryTexture(texture, NULL, NULL, w, h);
}

Enemy *create_enemy(SDL_Renderer *renderer, float x, float y)
{
    Enemy *enemy = (Enemy *)malloc(sizeof(Enemy));
    if (!enemy)
    {
        fprintf(stderr, "Failed to allocate memory for enemy\n");
        return NULL;
    }

    // Initialize position and physics
    enemy->x = x;
    enemy->y = y;
    enemy->velocity_x = 0;
    enemy->velocity_y = 0;
    enemy->speed = 450.0f; // pixels per second
    enemy->jump_force = -900.0f;
    enemy->gravity = 1500.0f;
    enemy->on_ground = false;

    // Initialize all texture pointers to NULL first
    enemy->idle_texture = NULL;
    enemy->walking_texture = NULL;
    enemy->jumping_texture = NULL;
    enemy->attack_texture = NULL;
    enemy->attack2_texture = NULL;
    enemy->attack3_texture = NULL;
    enemy->block_texture = NULL;
    enemy->hurt_texture = NULL;  // Add this
    enemy->death_texture = NULL; // Add this

    // Load textures
    enemy->idle_texture = load_texture(renderer, "assets/textures/Knight_1/Idle.bmp");
    enemy->walking_texture = load_texture(renderer, "assets/textures/Knight_1/Run.bmp");
    enemy->jumping_texture = load_texture(renderer, "assets/textures/Knight_1/Jump.bmp");
    enemy->attack_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 1.bmp");
    enemy->attack2_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 2.bmp");
    enemy->attack3_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 3.bmp");
    enemy->block_texture = load_texture(renderer, "assets/textures/Knight_1/Protect.bmp");
    enemy->hurt_texture = load_texture(renderer, "assets/textures/Knight_1/Hurt.bmp"); // Add this
    enemy->death_texture = load_texture(renderer, "assets/textures/Knight_1/Dead.bmp");

    // Check if all textures loaded successfully
    if (!enemy->idle_texture || !enemy->walking_texture || !enemy->jumping_texture ||
        !enemy->attack_texture || !enemy->attack2_texture || !enemy->attack3_texture ||
        !enemy->block_texture || !enemy->hurt_texture || !enemy->death_texture)
    {
        fprintf(stderr, "Failed to load one or more enemy textures\n");
        destroy_enemy(enemy);
        return NULL;
    }

    // Initialize animation
    enemy->current_frame = 0;
    enemy->frame_height = ENEMY_HEIGHT;
    enemy->last_frame_time = SDL_GetTicks();

    // Get initial frame width from idle texture
    int texture_width, texture_height;
    get_texture_dimensions(enemy->idle_texture, &texture_width, &texture_height);
    enemy->frame_width = texture_width / 1; // Idle has 4 frames
    enemy->frame_count = 1;
    enemy->frame_delay = 150;

    // Initialize rectangles
    enemy->src_rect.x = 0;
    enemy->src_rect.y = 0;
    enemy->src_rect.w = enemy->frame_width;
    enemy->src_rect.h = enemy->frame_height;

    enemy->dest_rect.x = (int)x;
    enemy->dest_rect.y = (int)y;
    enemy->dest_rect.w = enemy->frame_width;
    enemy->dest_rect.h = enemy->frame_height;

    // Initialize state
    enemy->state = ENEMY_IDLE;
    enemy->direction = L;
    enemy->is_attacking = false;
    enemy->is_blocking = false;
    enemy->attack_start_time = 0;
    enemy->attack_duration = 500; // milliseconds
    enemy->current_attack = 0;    // Initialize attack counter

    // Set initial state properly (this will set the correct frame_count and frame_delay)
    set_enemy_state(enemy, ENEMY_IDLE);

    return enemy;
}

void destroy_enemy(Enemy *enemy)
{
    if (!enemy)
        return;

    if (enemy->idle_texture)
        SDL_DestroyTexture(enemy->idle_texture);
    if (enemy->walking_texture)
        SDL_DestroyTexture(enemy->walking_texture);
    if (enemy->jumping_texture)
        SDL_DestroyTexture(enemy->jumping_texture);
    if (enemy->attack_texture)
        SDL_DestroyTexture(enemy->attack_texture);
    if (enemy->attack2_texture)
        SDL_DestroyTexture(enemy->attack2_texture);
    if (enemy->attack3_texture)
        SDL_DestroyTexture(enemy->attack3_texture);
    if (enemy->block_texture)
        SDL_DestroyTexture(enemy->block_texture);
    if (enemy->hurt_texture)
        SDL_DestroyTexture(enemy->hurt_texture); // Add this
    if (enemy->death_texture)
        SDL_DestroyTexture(enemy->death_texture); // Add this

    free(enemy);
}

void handle_enemy_ai(Enemy *enemy, Player *player, float delta_time)
{
    if (!enemy || !player || enemy->state == ENEMY_HURT || enemy->state == ENEMY_DEATH)
        return;

    // AI decision cooldown to prevent too frequent decisions
    static Uint32 last_decision_time = 0;
    Uint32 current_time = SDL_GetTicks();

    // Make decisions every 100-300ms
    if (current_time - last_decision_time < 100)
        return;

    // Calculate distance and direction to player
    float dx = player->x - enemy->x;
    float dy = player->y - enemy->y;
    float distance = sqrtf(dx * dx + dy * dy);

    // Determine if player is to the left or right
    enemy->direction = (dx < 0) ? L : R;

    // Reset velocity if not already moving
    if (!enemy->is_attacking && !enemy->is_blocking)
        enemy->velocity_x = 0;

    // AI State Machine with probability-based decisions
    float random_val = (float)rand() / RAND_MAX; // 0.0 to 1.0

    // Define AI behavior thresholds
    const float AGGRESSION_FACTOR = 1.0f; // Higher = more aggressive
    const float DEFENSE_FACTOR = 1.0f;    // Higher = more defensive
    const float MOBILITY_FACTOR = 1.0f;   // Higher = more mobile

    // Combat range thresholds
    const float MELEE_RANGE = 80.0f;
    const float MEDIUM_RANGE = 200.0f;
    const float FAR_RANGE = 400.0f;

    // DEFENSIVE BEHAVIOR - Check if should block
    if (player->is_attacking && distance < MELEE_RANGE * 1.5f)
    {
        if (random_val < DEFENSE_FACTOR && !enemy->is_attacking)
        {
            enemy->is_blocking = true;
            set_enemy_state(enemy, ENEMY_BLOCKING);
            last_decision_time = current_time;
            return;
        }
    }
    else if (enemy->is_blocking)
    {
        // Stop blocking when player stops attacking
        enemy->is_blocking = false;
        set_enemy_state(enemy, ENEMY_IDLE);
    }

    // OFFENSIVE BEHAVIOR - Attack when in range
    if (distance < MELEE_RANGE && !enemy->is_attacking && !enemy->is_blocking)
    {
        if (random_val < AGGRESSION_FACTOR)
        {
            enemy->is_attacking = true;
            enemy->attack_start_time = SDL_GetTicks();
            enemy->current_attack = rand() % 3; // Random attack pattern
            set_enemy_state(enemy, ENEMY_ATTACKING);
            last_decision_time = current_time;
            return;
        }
    }

    // MOVEMENT BEHAVIOR
    if (!enemy->is_attacking && !enemy->is_blocking)
    {
        // Close distance if too far
        if (distance > MEDIUM_RANGE)
        {
            if (random_val < MOBILITY_FACTOR)
            {
                // Move towards player
                enemy->velocity_x = (dx > 0) ? enemy->speed : -enemy->speed;
                if (enemy->on_ground)
                    set_enemy_state(enemy, ENEMY_WALKING);
            }
        }
        // Maintain optimal combat distance
        else if (distance > MELEE_RANGE && distance < MEDIUM_RANGE)
        {
            if (random_val < MOBILITY_FACTOR * 0.8f)
            {
                // Approach cautiously
                enemy->velocity_x = (dx > 0) ? enemy->speed * 0.7f : -enemy->speed * 0.7f;
                if (enemy->on_ground)
                    set_enemy_state(enemy, ENEMY_WALKING);
            }
        }
        // Back away if too close (defensive maneuver)
        else if (distance < MELEE_RANGE * 0.5f)
        {
            if (random_val < (1.0f - AGGRESSION_FACTOR))
            {
                // Move away from player
                enemy->velocity_x = (dx > 0) ? -enemy->speed : enemy->speed;
                if (enemy->on_ground)
                    set_enemy_state(enemy, ENEMY_WALKING);
            }
        }

        // JUMPING BEHAVIOR
        // Jump to reach player if they're higher
        if (dy < -50 && enemy->on_ground && random_val < MOBILITY_FACTOR)
        {
            enemy->velocity_y = enemy->jump_force;
            enemy->on_ground = false;
            set_enemy_state(enemy, ENEMY_JUMPING);
        }
        // Dodge jump - occasionally jump to avoid attacks
        else if (player->is_attacking && distance < MEDIUM_RANGE && enemy->on_ground)
        {
            if (random_val < MOBILITY_FACTOR * 0.3f)
            {
                enemy->velocity_y = enemy->jump_force;
                enemy->on_ground = false;
                set_enemy_state(enemy, ENEMY_JUMPING);

                // Also move backward while jumping
                enemy->velocity_x = (dx > 0) ? -enemy->speed : enemy->speed;
            }
        }
    }

    // Return to idle if not doing anything
    if (enemy->velocity_x == 0 && enemy->on_ground &&
        !enemy->is_attacking && !enemy->is_blocking &&
        enemy->state == ENEMY_WALKING)
    {
        set_enemy_state(enemy, ENEMY_IDLE);
    }

    // Handle air state
    if (!enemy->on_ground && enemy->state != ENEMY_JUMPING &&
        !enemy->is_attacking && !enemy->is_blocking)
    {
        set_enemy_state(enemy, ENEMY_JUMPING);
    }

    last_decision_time = current_time;
}

void update_enemy(Enemy *enemy, Uint32 delta_time)
{
    if (!enemy)
        return;

    float dt = delta_time / 1000.0f; // Convert to seconds

    // Update position
    enemy->x += enemy->velocity_x * dt;
    enemy->y += enemy->velocity_y * dt;

    // Apply gravity
    if (!enemy->on_ground)
    {
        enemy->velocity_y += enemy->gravity * dt;
    }

    // Check ground collision (simple ground check)
    if (enemy->y >= GROUND_LEVEL)
    {
        enemy->y = GROUND_LEVEL;
        enemy->velocity_y = 0;

        if (!enemy->on_ground) // Just landed
        {
            enemy->on_ground = true;
            if (enemy->state == ENEMY_JUMPING)
            {
                set_enemy_state(enemy, ENEMY_IDLE);
            }
        }
    }
    else
    {
        enemy->on_ground = false;
    }

    // Update attack state
    if (enemy->is_attacking)
    {
        Uint32 current_time = SDL_GetTicks();
        if (current_time - enemy->attack_start_time >= enemy->attack_duration)
        {
            enemy->is_attacking = false;
            set_enemy_state(enemy, ENEMY_IDLE);
        }
    }

    // Update animation
    Uint32 current_time = SDL_GetTicks();
    if (current_time - enemy->last_frame_time >= enemy->frame_delay)
    {
        if (enemy->state == ENEMY_DEATH)
        {
            /* Let the animation run once and then freeze                */
            if (enemy->current_frame < enemy->frame_count - 1)
            {
                enemy->current_frame++;
                enemy->src_rect.x = enemy->current_frame * enemy->frame_width;
            }
            /* nothing happens once we are on the last frame             */
        }
        else if (enemy->state == ENEMY_ATTACKING)
        {
            if (enemy->current_frame < enemy->frame_count - 1)
            {
                enemy->current_frame++;
            }
            else
            {
                /* animation finished; if the combat code has already
                   set is_attacking = false, drop back to idle         */
                if (!enemy->is_attacking)
                    set_enemy_state(enemy, ENEMY_IDLE);
            }
            enemy->src_rect.x = enemy->current_frame * enemy->frame_width;
        }
        else /* every other state keeps looping as usual                 */
        {
            enemy->current_frame = (enemy->current_frame + 1) % enemy->frame_count;
            enemy->src_rect.x = enemy->current_frame * enemy->frame_width;
        }

        enemy->last_frame_time = current_time;
    }

    // Update destination rectangle
    enemy->dest_rect.x = (int)enemy->x;
    enemy->dest_rect.y = (int)enemy->y;
    enemy->dest_rect.w = enemy->frame_width;
    enemy->dest_rect.h = enemy->frame_height;

    // Keep enemy on screen
    if (enemy->x < 0)
        enemy->x = 0;
    if (enemy->x > 1280 - enemy->frame_width)
        enemy->x = 1280 - enemy->frame_width;
}

void render_enemy(SDL_Renderer *renderer, Enemy *enemy)
{
    if (!enemy || !renderer)
        return;

    SDL_Texture *current_texture = NULL;

    // Select appropriate texture based on state
    switch (enemy->state)
    {
    case ENEMY_IDLE:
        current_texture = enemy->idle_texture;
        break;
    case ENEMY_WALKING:
        current_texture = enemy->walking_texture;
        break;
    case ENEMY_JUMPING:
        current_texture = enemy->jumping_texture;
        break;
    case ENEMY_ATTACKING:
        // Select attack texture based on current attack cycle
        switch (enemy->current_attack)
        {
        case 0:
            current_texture = enemy->attack_texture;
            break;
        case 1:
            current_texture = enemy->attack2_texture;
            break;
        case 2:
            current_texture = enemy->attack3_texture;
            break;
        }
        break;
    case ENEMY_BLOCKING:
        current_texture = enemy->block_texture;
        break;
    case ENEMY_DEATH:
        current_texture = enemy->death_texture;
        break;
    case ENEMY_HURT:
        current_texture = enemy->hurt_texture;
        break;
    }

    if (current_texture == enemy->death_texture)
    {
        if (enemy->current_frame >= enemy->frame_count - 1)
        {
            enemy->current_frame = enemy->frame_count - 1;
        } // last frame of death animation
    }

    if (!current_texture)
    {
        fprintf(stderr, "No texture available for current enemy state %d\n", enemy->state);
        return;
    }

    // Render with flip based on direction
    SDL_RendererFlip flip = (enemy->direction == L) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    if (SDL_RenderCopyEx(renderer, current_texture, &enemy->src_rect, &enemy->dest_rect,
                         0, NULL, flip) != 0)
    {
        fprintf(stderr, "Failed to render enemy: %s\n", SDL_GetError());
    }
}

void set_enemy_state(Enemy *enemy, EnemyState new_state)
{
    if (!enemy)
        return;

    // Only change state if it's different (except for attacking which can cycle)
    if (enemy->state == new_state && new_state != ENEMY_ATTACKING)
        return;

    enemy->state = new_state;
    enemy->current_frame = 0;                // Reset animation
    enemy->last_frame_time = SDL_GetTicks(); // Reset animation timer

    SDL_Texture *texture = NULL;
    int texture_width, texture_height;

    // Set frame count and delay based on the new state, and update frame width
    switch (new_state)
    {
    case ENEMY_IDLE:
        texture = enemy->idle_texture;
        enemy->frame_count = 1;
        enemy->frame_delay = 150; // Slower for idle
        break;
    case ENEMY_WALKING:
        texture = enemy->walking_texture;
        enemy->frame_count = 7;
        enemy->frame_delay = 100;
        break;
    case ENEMY_JUMPING:
        texture = enemy->jumping_texture;
        enemy->frame_count = 6;
        enemy->frame_delay = 100;
        break;
    case ENEMY_ATTACKING:
        // Set frame count based on which attack animation
        switch (enemy->current_attack)
        {
        case 0:
            texture = enemy->attack_texture;
            enemy->frame_count = 5; // Attack 1 has 5 frames
            break;
        case 1:
            texture = enemy->attack2_texture;
            enemy->frame_count = 4; // Attack 2 has 4 frames
            break;
        case 2:
            texture = enemy->attack3_texture;
            enemy->frame_count = 4; // Attack 3 has 4 frames
            break;
        }
        enemy->frame_delay = 80; // Faster animation for attacks
        break;
    case ENEMY_BLOCKING:
        texture = enemy->block_texture;
        enemy->frame_count = 1; // Single frame for blocking
        enemy->frame_delay = 100;
        break;
    case ENEMY_DEATH:
        texture = enemy->death_texture;
        enemy->frame_count = 6; // Adjust based on your death animation frames
        enemy->frame_delay = 150;
        break;
    case ENEMY_HURT:
        texture = enemy->hurt_texture;
        enemy->frame_count = 2; // Adjust based on your hurt animation frames
        enemy->frame_delay = 100;
        break;
    }

    // Update frame width based on the texture
    if (texture)
    {
        get_texture_dimensions(texture, &texture_width, &texture_height);
        enemy->frame_width = texture_width / enemy->frame_count;

        // Update rectangles with new dimensions
        enemy->src_rect.w = enemy->frame_width;
        enemy->dest_rect.w = enemy->frame_width;
    }

    // Reset source rectangle to first frame
    enemy->src_rect.x = 0;
}