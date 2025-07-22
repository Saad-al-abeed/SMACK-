#include "player.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define GROUND_LEVEL 375 // Adjust based on your map
#define PLAYER_HEIGHT 258

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

Player *create_player(SDL_Renderer *renderer, float x, float y)
{
    Player *player = (Player *)malloc(sizeof(Player));
    if (!player)
    {
        fprintf(stderr, "Failed to allocate memory for player\n");
        return NULL;
    }

    // Initialize position and physics
    player->x = x;
    player->y = y;
    player->velocity_x = 0;
    player->velocity_y = 0;
    player->speed = 450.0f; // pixels per second
    player->jump_force = -900.0f;
    player->gravity = 1500.0f;
    player->on_ground = false;

    // Initialize all texture pointers to NULL first
    player->idle_texture = NULL;
    player->walking_texture = NULL;
    player->jumping_texture = NULL;
    player->attack_texture = NULL;
    player->attack2_texture = NULL;
    player->attack3_texture = NULL;
    player->block_texture = NULL;

    // Load textures
    player->idle_texture = load_texture(renderer, "assets/textures/Knight_1/Idle.bmp");
    player->walking_texture = load_texture(renderer, "assets/textures/Knight_1/Run.bmp");
    player->jumping_texture = load_texture(renderer, "assets/textures/Knight_1/Jump.bmp");
    player->attack_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 1.bmp");
    player->attack2_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 2.bmp");
    player->attack3_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 3.bmp");
    player->block_texture = load_texture(renderer, "assets/textures/Knight_1/Protect.bmp");

    // Check if all textures loaded successfully
    if (!player->idle_texture || !player->walking_texture || !player->jumping_texture ||
        !player->attack_texture || !player->attack2_texture || !player->attack3_texture ||
        !player->block_texture)
    {
        fprintf(stderr, "Failed to load one or more player textures\n");
        destroy_player(player);
        return NULL;
    }

    // Initialize animation
    player->current_frame = 0;
    player->frame_height = PLAYER_HEIGHT;
    player->last_frame_time = SDL_GetTicks();

    // Get initial frame width from idle texture
    int texture_width, texture_height;
    get_texture_dimensions(player->idle_texture, &texture_width, &texture_height);
    player->frame_width = texture_width / 1; // Idle has 4 frames
    player->frame_count = 1;
    player->frame_delay = 150;

    // Initialize rectangles
    player->src_rect.x = 0;
    player->src_rect.y = 0;
    player->src_rect.w = player->frame_width;
    player->src_rect.h = player->frame_height;

    player->dest_rect.x = (int)x;
    player->dest_rect.y = (int)y;
    player->dest_rect.w = player->frame_width;
    player->dest_rect.h = player->frame_height;

    // Initialize state
    player->state = PLAYER_IDLE;
    player->direction = FACING_RIGHT;
    player->is_attacking = false;
    player->is_blocking = false;
    player->attack_start_time = 0;
    player->attack_duration = 500; // milliseconds
    player->current_attack = 0;    // Initialize attack counter

    // Set initial state properly (this will set the correct frame_count and frame_delay)
    set_player_state(player, PLAYER_IDLE);

    return player;
}

void destroy_player(Player *player)
{
    if (!player)
        return;

    if (player->idle_texture)
        SDL_DestroyTexture(player->idle_texture);
    if (player->walking_texture)
        SDL_DestroyTexture(player->walking_texture);
    if (player->jumping_texture)
        SDL_DestroyTexture(player->jumping_texture);
    if (player->attack_texture)
        SDL_DestroyTexture(player->attack_texture);
    if (player->attack2_texture)
        SDL_DestroyTexture(player->attack2_texture);
    if (player->attack3_texture)
        SDL_DestroyTexture(player->attack3_texture);
    if (player->block_texture)
        SDL_DestroyTexture(player->block_texture);

    free(player);
}

void handle_player_input(Player *player, const Uint8 *keystate)
{
    if (!player)
        return;

    // Store previous velocity for movement state detection
    float prev_velocity_x = player->velocity_x;

    // Reset horizontal velocity
    player->velocity_x = 0;

    // Movement - only walk while key is held
    if (keystate[SDL_SCANCODE_A] && !player->is_attacking && !player->is_blocking)
    {
        player->velocity_x = -player->speed;
        player->direction = FACING_LEFT;
        if (player->state != PLAYER_JUMPING && player->on_ground)
        {
            set_player_state(player, PLAYER_WALKING);
        }
    }
    else if (keystate[SDL_SCANCODE_D] && !player->is_attacking && !player->is_blocking)
    {
        player->velocity_x = player->speed;
        player->direction = FACING_RIGHT;
        if (player->state != PLAYER_JUMPING && player->on_ground)
        {
            set_player_state(player, PLAYER_WALKING);
        }
    }
    else
    {
        // Return to idle when movement keys are released
        if (player->state == PLAYER_WALKING && player->on_ground && !player->is_attacking && !player->is_blocking)
        {
            set_player_state(player, PLAYER_IDLE);
        }
    }

    // Jump
    if (keystate[SDL_SCANCODE_SPACE] && player->on_ground && !player->is_attacking && !player->is_blocking)
    {
        player->velocity_y = player->jump_force;
        player->on_ground = false;
        set_player_state(player, PLAYER_JUMPING);
    }

    // Attack - cycle through 3 attack animations
    static bool w_was_pressed = false;
    if (keystate[SDL_SCANCODE_W] && !player->is_attacking && !player->is_blocking )
    {
        if (!w_was_pressed) // Only trigger on key press, not hold
        {
            player->is_attacking = true;
            player->attack_start_time = SDL_GetTicks();
            player->current_attack = (player->current_attack + 1) % 3; // Cycle through 0, 1, 2
            set_player_state(player, PLAYER_ATTACKING);
            w_was_pressed = true;
        }
    }
    else
    {
        w_was_pressed = false;
    }

    // Block - only block while key is held
    if (keystate[SDL_SCANCODE_S] && !player->is_attacking )
    {
        if (!player->is_blocking)
        {
            player->is_blocking = true;
            set_player_state(player, PLAYER_BLOCKING);
        }
    }
    else
    {
        if (player->is_blocking)
        {
            player->is_blocking = false;
            set_player_state(player, PLAYER_IDLE);
        }
    }

    //on air animation
    if (!player->on_ground && player->state != PLAYER_JUMPING && !player->is_attacking && !player->is_blocking)
    {
        set_player_state(player, PLAYER_JUMPING);
    }
}

void update_player(Player *player, Uint32 delta_time)
{
    if (!player)
        return;

    float dt = delta_time / 1000.0f; // Convert to seconds

    // Update position
    player->x += player->velocity_x * dt;
    player->y += player->velocity_y * dt;

    // Apply gravity
    if (!player->on_ground)
    {
        player->velocity_y += player->gravity * dt;
    }

    // Check ground collision (simple ground check)
    if (player->y >= GROUND_LEVEL)
    {
        player->y = GROUND_LEVEL;
        player->velocity_y = 0;

        if (!player->on_ground) // Just landed
        {
            player->on_ground = true;
            if (player->state == PLAYER_JUMPING)
            {
                set_player_state(player, PLAYER_IDLE);
            }
        }
    }
    else
    {
        player->on_ground = false;
    }

    // Update attack state
    if (player->is_attacking)
    {
        Uint32 current_time = SDL_GetTicks();
        if (current_time - player->attack_start_time >= player->attack_duration)
        {
            player->is_attacking = false;
            set_player_state(player, PLAYER_IDLE);
        }
    }

    // Update animation
    Uint32 current_time = SDL_GetTicks();
    if (current_time - player->last_frame_time >= player->frame_delay)
    {
        player->current_frame = (player->current_frame + 1) % player->frame_count;
        player->last_frame_time = current_time;

        // Update source rectangle for animation
        player->src_rect.x = player->current_frame * player->frame_width;
    }

    // Update destination rectangle
    player->dest_rect.x = (int)player->x;
    player->dest_rect.y = (int)player->y;
    player->dest_rect.w = player->frame_width;
    player->dest_rect.h = player->frame_height;

    // Keep player on screen
    if (player->x < 0)
        player->x = 0;
    if (player->x > 1280 - player->frame_width)
        player->x = 1280 - player->frame_width;
}

void render_player(SDL_Renderer *renderer, Player *player)
{
    if (!player || !renderer)
        return;

    SDL_Texture *current_texture = NULL;

    // Select appropriate texture based on state
    switch (player->state)
    {
    case PLAYER_IDLE:
        current_texture = player->idle_texture;
        break;
    case PLAYER_WALKING:
        current_texture = player->walking_texture;
        break;
    case PLAYER_JUMPING:
        current_texture = player->jumping_texture;
        break;
    case PLAYER_ATTACKING:
        // Select attack texture based on current attack cycle
        switch (player->current_attack)
        {
        case 0:
            current_texture = player->attack_texture;
            break;
        case 1:
            current_texture = player->attack2_texture;
            break;
        case 2:
            current_texture = player->attack3_texture;
            break;
        }
        break;
    case PLAYER_BLOCKING:
        current_texture = player->block_texture;
        break;
    }

    if (!current_texture)
    {
        fprintf(stderr, "No texture available for current player state %d\n", player->state);
        return;
    }

    // Render with flip based on direction
    SDL_RendererFlip flip = (player->direction == FACING_LEFT) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    if (SDL_RenderCopyEx(renderer, current_texture, &player->src_rect, &player->dest_rect,
                         0, NULL, flip) != 0)
    {
        fprintf(stderr, "Failed to render player: %s\n", SDL_GetError());
    }
}

void set_player_state(Player *player, PlayerState new_state)
{
    if (!player)
        return;

    // Only change state if it's different (except for attacking which can cycle)
    if (player->state == new_state && new_state != PLAYER_ATTACKING)
        return;

    player->state = new_state;
    player->current_frame = 0;                // Reset animation
    player->last_frame_time = SDL_GetTicks(); // Reset animation timer

    SDL_Texture *texture = NULL;
    int texture_width, texture_height;

    // Set frame count and delay based on the new state, and update frame width
    switch (new_state)
    {
    case PLAYER_IDLE:
        texture = player->idle_texture;
        player->frame_count = 1;
        player->frame_delay = 150; // Slower for idle
        break;
    case PLAYER_WALKING:
        texture = player->walking_texture;
        player->frame_count = 7;
        player->frame_delay = 100;
        break;
    case PLAYER_JUMPING:
        texture = player->jumping_texture;
        player->frame_count = 6;
        player->frame_delay = 100;
        break;
    case PLAYER_ATTACKING:
        // Set frame count based on which attack animation
        switch (player->current_attack)
        {
        case 0:
            texture = player->attack_texture;
            player->frame_count = 5; // Attack 1 has 5 frames
            break;
        case 1:
            texture = player->attack2_texture;
            player->frame_count = 4; // Attack 2 has 4 frames
            break;
        case 2:
            texture = player->attack3_texture;
            player->frame_count = 4; // Attack 3 has 4 frames
            break;
        }
        player->frame_delay = 80; // Faster animation for attacks
        break;
    case PLAYER_BLOCKING:
        texture = player->block_texture;
        player->frame_count = 1; // Single frame for blocking
        player->frame_delay = 100;
        break;
    }

    // Update frame width based on the texture
    if (texture)
    {
        get_texture_dimensions(texture, &texture_width, &texture_height);
        player->frame_width = texture_width / player->frame_count;

        // Update rectangles with new dimensions
        player->src_rect.w = player->frame_width;
        player->dest_rect.w = player->frame_width;
    }

    // Reset source rectangle to first frame
    player->src_rect.x = 0;
}