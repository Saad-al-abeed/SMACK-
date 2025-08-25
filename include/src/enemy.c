#include "enemy.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GROUND_LEVEL 375
#define ENEMY_HEIGHT 258

// --- Helper Functions ---
static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface) { fprintf(stderr, "Failed to load BMP %s: %s\n", path, SDL_GetError()); return NULL; }
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 255, 0, 255));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) { fprintf(stderr, "Failed to create texture from %s: %s\n", path, SDL_GetError()); }
    return texture;
}

static void get_texture_dimensions(SDL_Texture *texture, int *w, int *h) {
    SDL_QueryTexture(texture, NULL, NULL, w, h);
}


// --- Create/Destroy Functions ---
Enemy *create_enemy(SDL_Renderer *renderer, float x, float y) {
    Enemy *enemy = (Enemy *)malloc(sizeof(Enemy));
    if (!enemy) return NULL;

    // Initialize all fields to zero/false/NULL to prevent garbage values
    *enemy = (Enemy){0};

    // Set initial physics and position values
    enemy->x = x;
    enemy->y = y;
    enemy->speed = 350.0f;
    enemy->gravity = 1500.0f;
    enemy->attack_duration = 500; // ms
    enemy->direction = L;

    // Load all textures
    enemy->idle_texture = load_texture(renderer, "assets/textures/Knight_1/Idle.bmp");
    enemy->walking_texture = load_texture(renderer, "assets/textures/Knight_1/Run.bmp");
    enemy->jumping_texture = load_texture(renderer, "assets/textures/Knight_1/Jump.bmp");
    enemy->attack_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 1.bmp");
    enemy->attack2_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 2.bmp");
    enemy->attack3_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 3.bmp");
    enemy->block_texture = load_texture(renderer, "assets/textures/Knight_1/Protect.bmp");
    enemy->hurt_texture = load_texture(renderer, "assets/textures/Knight_1/Hurt.bmp");
    enemy->death_texture = load_texture(renderer, "assets/textures/Knight_1/Dead.bmp");
    
    // Set initial state which also calculates initial frame dimensions
    set_enemy_state(enemy, ENEMY_IDLE);

    //Initialize both src and dest rects correctly after sizes are known
    enemy->dest_rect = (SDL_Rect){(int)x, (int)y, (int)enemy->frame_width, ENEMY_HEIGHT};
    enemy->src_rect = (SDL_Rect){0, 0, (int)enemy->frame_width, ENEMY_HEIGHT};

    return enemy;
}

void destroy_enemy(Enemy *enemy) {
    if (!enemy) return;
    SDL_DestroyTexture(enemy->idle_texture);
    SDL_DestroyTexture(enemy->walking_texture);
    SDL_DestroyTexture(enemy->jumping_texture);
    SDL_DestroyTexture(enemy->attack_texture);
    SDL_DestroyTexture(enemy->attack2_texture);
    SDL_DestroyTexture(enemy->attack3_texture);
    SDL_DestroyTexture(enemy->block_texture);
    SDL_DestroyTexture(enemy->hurt_texture);
    SDL_DestroyTexture(enemy->death_texture);
    free(enemy);
}


// =================================================================================
//
//          FINAL, SIMPLIFIED, AND AGGRESSIVE AI LOGIC
//
// =================================================================================
void handle_enemy_ai(Enemy *enemy, Player *player, float delta_time)
{
    if (!enemy || !player || enemy->state == ENEMY_HURT || enemy->state == ENEMY_DEATH) {
        enemy->velocity_x = 0;
        return;
    }

    // --- Senses ---
    float distance_x = player->x - enemy->x;
    float abs_distance = fabs(distance_x);
    enemy->direction = (distance_x > 0) ? R : L;
    
    bool is_player_facing_enemy = ((player->direction == FACING_RIGHT && distance_x < 0) ||
                               (player->direction == FACING_LEFT && distance_x > 0));

    // --- Constants ---
    const int ATTACK_RANGE = 200;
    const int ATTACK_STOP_DISTANCE = 400;  // Distance to stop and prepare attack
    const int REPOSITION_DISTANCE = 200;

    // If currently attacking, let the attack finish
    if (enemy->is_attacking) {
        if (SDL_GetTicks() - enemy->attack_start_time < enemy->attack_duration) {
            enemy->velocity_x = 0;
            return;
        }
        enemy->is_attacking = false;
    }

    // **Priority 1: DEFEND**
    if (player->is_attacking && is_player_facing_enemy && abs_distance < ATTACK_RANGE + 20) {
        enemy->velocity_x = 0;
        set_enemy_state(enemy, ENEMY_BLOCKING);
        return;
    }

    // **Priority 2: ATTACK**
    bool player_is_vulnerable = (!player->is_blocking || !is_player_facing_enemy);
    if (abs_distance <= ATTACK_RANGE && player_is_vulnerable) {
        // Stop moving when in attack range
        if (abs_distance > ATTACK_STOP_DISTANCE) {
            enemy->velocity_x = (enemy->direction == R) ? enemy->speed : -enemy->speed;
            set_enemy_state(enemy, ENEMY_WALKING);
        } 
        else {
            enemy->velocity_x = 0;
            enemy->is_attacking = true;
            enemy->current_attack = rand() % 3;
            enemy->attack_start_time = SDL_GetTicks();
            set_enemy_state(enemy, ENEMY_ATTACKING);
        }
        return;
    }

    // **Priority 3: REPOSITION**
    // if (abs_distance < REPOSITION_DISTANCE) {
    //     enemy->velocity_x = (enemy->direction == R) ? -enemy->speed * 0.7f : enemy->speed * 0.7f;
    //     set_enemy_state(enemy, ENEMY_WALKING);
    //     return;
    // }

    // **Priority 3: CHASE**
    enemy->velocity_x = (enemy->direction == R) ? enemy->speed : -enemy->speed;
    set_enemy_state(enemy, ENEMY_WALKING);
}


// --- Update, Render, SetState ---
void update_enemy(Enemy *enemy, Uint32 delta_time)
{
    if (!enemy) return;
    float dt = delta_time / 1000.0f;
    if (enemy->state == ENEMY_DEATH) { 
        enemy->velocity_x = 0; 
    }
    enemy->x += enemy->velocity_x * dt;
    if (!enemy->on_ground) { 
        enemy->velocity_y += enemy->gravity * dt; 
    }
    enemy->y += enemy->velocity_y * dt;
    if (enemy->y >= GROUND_LEVEL) {
        enemy->y = GROUND_LEVEL;
        enemy->velocity_y = 0;
        enemy->on_ground = true;
        if (enemy->state == ENEMY_JUMPING) { 
            set_enemy_state(enemy, ENEMY_IDLE); 
        }
    } 
    else { 
        enemy->on_ground = false; 
    }
    Uint32 current_time = SDL_GetTicks();
    if (current_time - enemy->last_frame_time >= enemy->frame_delay) {
        if (enemy->state == ENEMY_DEATH || enemy->state == ENEMY_ATTACKING) {
            if (enemy->current_frame < enemy->frame_count - 1) { 
                enemy->current_frame++; 
            }
        } 
        else {
            enemy->current_frame = (enemy->current_frame + 1) % enemy->frame_count;
        }
        enemy->last_frame_time = current_time;
    }
    enemy->src_rect.x = enemy->current_frame * enemy->frame_width;
    enemy->dest_rect.x = (int)enemy->x;
    enemy->dest_rect.y = (int)enemy->y;
}

void render_enemy(SDL_Renderer *renderer, Enemy *enemy) {
    if (!enemy || !renderer) return;
    SDL_Texture *current_texture = enemy->idle_texture;
    switch (enemy->state) {
        case ENEMY_IDLE:    current_texture = enemy->idle_texture; break;
        case ENEMY_WALKING: current_texture = enemy->walking_texture; break;
        case ENEMY_JUMPING: current_texture = enemy->jumping_texture; break;
        case ENEMY_BLOCKING:current_texture = enemy->block_texture; break;
        case ENEMY_HURT:    current_texture = enemy->hurt_texture; break;
        case ENEMY_DEATH:   current_texture = enemy->death_texture; break;
        case ENEMY_ATTACKING:
            switch (enemy->current_attack) {
                case 0: current_texture = enemy->attack_texture; break;
                case 1: current_texture = enemy->attack2_texture; break;
                case 2: current_texture = enemy->attack3_texture; break;
            } break;
    }
    SDL_RendererFlip flip = (enemy->direction == L) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, current_texture, &enemy->src_rect, &enemy->dest_rect, 0, NULL, flip);
}

void set_enemy_state(Enemy *enemy, EnemyState new_state) {
    if (!enemy || enemy->state == new_state) return;

    enemy->state = new_state;
    enemy->current_frame = 0;
    SDL_Texture *texture = NULL;
    int texture_width;

    switch (new_state) {
        case ENEMY_IDLE:
            texture = enemy->idle_texture; enemy->frame_count = 1; enemy->frame_delay = 150; break;
        case ENEMY_WALKING:
            texture = enemy->walking_texture; enemy->frame_count = 7; enemy->frame_delay = 100; break;
        case ENEMY_JUMPING:
            texture = enemy->jumping_texture; enemy->frame_count = 6; enemy->frame_delay = 100; break;
        case ENEMY_ATTACKING:
            switch (enemy->current_attack) {
                case 0: texture = enemy->attack_texture; enemy->frame_count = 5; break;
                case 1: texture = enemy->attack2_texture; enemy->frame_count = 4; break;
                case 2: texture = enemy->attack3_texture; enemy->frame_count = 4; break;
            } enemy->frame_delay = 10; break;
        case ENEMY_BLOCKING:
            texture = enemy->block_texture; enemy->frame_count = 1; enemy->frame_delay = 100; break;
        case ENEMY_HURT:
            texture = enemy->hurt_texture; enemy->frame_count = 2; enemy->frame_delay = 100; break;
        case ENEMY_DEATH:
            texture = enemy->death_texture; enemy->frame_count = 6; enemy->frame_delay = 150; break;
    }

    if (texture) {
        get_texture_dimensions(texture, &texture_width, NULL);
        enemy->frame_width = texture_width / enemy->frame_count;
        enemy->frame_height = ENEMY_HEIGHT;
        enemy->src_rect.w = enemy->frame_width;
        enemy->dest_rect.w = enemy->frame_width;
        enemy->src_rect.h = enemy->frame_height;
        enemy->dest_rect.h = enemy->frame_height;
    }
}