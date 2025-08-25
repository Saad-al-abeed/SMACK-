#include "player2.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define GROUND_LEVEL 375
#define PLAYER2_HEIGHT 258

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

Player2 *create_player2(SDL_Renderer *renderer, float x, float y)
{
    Player2 *player2 = (Player2 *)malloc(sizeof(Player2));
    if (!player2)
    {
        fprintf(stderr, "Failed to allocate memory for player2\n");
        return NULL;
    }

    // Initialize position and physics
    player2->x = x;
    player2->y = y;
    player2->velocity_x = 0;
    player2->velocity_y = 0;
    player2->speed = 450.0f; // pixels per second
    player2->jump_force = -900.0f;
    player2->gravity = 1500.0f;
    player2->on_ground = false;

    // Initialize all texture pointers to NULL first
    player2->idle_texture = NULL;
    player2->walking_texture = NULL;
    player2->jumping_texture = NULL;
    player2->attack_texture = NULL;
    player2->attack2_texture = NULL;
    player2->attack3_texture = NULL;
    player2->block_texture = NULL;
    player2->hurt_texture = NULL; 
    player2->death_texture = NULL; 

    // Load textures
    player2->idle_texture = load_texture(renderer, "assets/textures/Knight_1/Idle.bmp");
    player2->walking_texture = load_texture(renderer, "assets/textures/Knight_1/Run.bmp");
    player2->jumping_texture = load_texture(renderer, "assets/textures/Knight_1/Jump.bmp");
    player2->attack_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 1.bmp");
    player2->attack2_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 2.bmp");
    player2->attack3_texture = load_texture(renderer, "assets/textures/Knight_1/Attack 3.bmp");
    player2->block_texture = load_texture(renderer, "assets/textures/Knight_1/Protect.bmp");
    player2->hurt_texture = load_texture(renderer, "assets/textures/Knight_1/Hurt.bmp"); // Add this
    player2->death_texture = load_texture(renderer, "assets/textures/Knight_1/Dead.bmp");

    // Check if all textures loaded successfully
    if (!player2->idle_texture || !player2->walking_texture || !player2->jumping_texture ||
        !player2->attack_texture || !player2->attack2_texture || !player2->attack3_texture ||
        !player2->block_texture || !player2->hurt_texture || !player2->death_texture)
    {
        fprintf(stderr, "Failed to load one or more player2 textures\n");
        destroy_player2(player2);
        return NULL;
    }

    // Initialize animation
    player2->current_frame = 0;
    player2->frame_height = PLAYER2_HEIGHT;
    player2->last_frame_time = SDL_GetTicks();

    // Get initial frame width from idle texture
    int texture_width, texture_height;
    get_texture_dimensions(player2->idle_texture, &texture_width, &texture_height);
    player2->frame_width = texture_width / 1; // Idle has 4 frames
    player2->frame_count = 1;
    player2->frame_delay = 150;

    // Initialize rectangles
    player2->src_rect.x = 0;
    player2->src_rect.y = 0;
    player2->src_rect.w = player2->frame_width;
    player2->src_rect.h = player2->frame_height;

    player2->dest_rect.x = (int)x;
    player2->dest_rect.y = (int)y;
    player2->dest_rect.w = player2->frame_width;
    player2->dest_rect.h = player2->frame_height;

    // Initialize state
    player2->state = PLAYER2_IDLE;
    player2->direction = LEFT;
    player2->is_attacking = false;
    player2->is_blocking = false;
    player2->attack_start_time = 0;
    player2->attack_duration = 500; // milliseconds
    player2->current_attack = 0;    // Initialize attack counter

    // Set initial state properly (this will set the correct frame_count and frame_delay)
    set_player2_state(player2, PLAYER2_IDLE);

    return player2;
}

void destroy_player2(Player2 *player2)
{
    if (!player2)
        return;

    if (player2->idle_texture)
        SDL_DestroyTexture(player2->idle_texture);
    if (player2->walking_texture)
        SDL_DestroyTexture(player2->walking_texture);
    if (player2->jumping_texture)
        SDL_DestroyTexture(player2->jumping_texture);
    if (player2->attack_texture)
        SDL_DestroyTexture(player2->attack_texture);
    if (player2->attack2_texture)
        SDL_DestroyTexture(player2->attack2_texture);
    if (player2->attack3_texture)
        SDL_DestroyTexture(player2->attack3_texture);
    if (player2->block_texture)
        SDL_DestroyTexture(player2->block_texture);
    if (player2->hurt_texture)
        SDL_DestroyTexture(player2->hurt_texture);
    if (player2->death_texture)
        SDL_DestroyTexture(player2->death_texture);

    free(player2);
}

void handle_player2_input(Player2 *player2, const Uint8 *keystate)
{
    if (!player2 || player2->state == PLAYER2_HURT || player2->state == PLAYER2_DEATH)
        return;

    // Store previous velocity for movement state detection
    float prev_velocity_x = player2->velocity_x;

    // Reset horizontal velocity
    player2->velocity_x = 0;

    // Movement with arrow keys
    if (keystate[SDL_SCANCODE_LEFT] && !player2->is_attacking && !player2->is_blocking)
    {
        player2->velocity_x = -player2->speed;
        player2->direction = LEFT;
        if (player2->state != PLAYER2_JUMPING && player2->on_ground)
        {
            set_player2_state(player2, PLAYER2_WALKING);
        }
    }
    else if (keystate[SDL_SCANCODE_RIGHT] && !player2->is_attacking && !player2->is_blocking)
    {
        player2->velocity_x = player2->speed;
        player2->direction = RIGHT;
        if (player2->state != PLAYER2_JUMPING && player2->on_ground)
        {
            set_player2_state(player2, PLAYER2_WALKING);
        }
    }
    else
    {
        // Return to idle when movement keys are released
        if (player2->state == PLAYER2_WALKING && player2->on_ground && !player2->is_attacking && !player2->is_blocking)
        {
            set_player2_state(player2, PLAYER2_IDLE);
        }
    }

    // Jump with numpad enter
    if (keystate[SDL_SCANCODE_KP_ENTER] && player2->on_ground && !player2->is_attacking && !player2->is_blocking)
    {
        player2->velocity_y = player2->jump_force;
        player2->on_ground = false;
        set_player2_state(player2, PLAYER2_JUMPING);
    }

    // Attack with up arrow - cycle through 3 attack animations
    static bool up_was_pressed = false;
    if (keystate[SDL_SCANCODE_UP] && !player2->is_attacking && !player2->is_blocking)
    {
        if (!up_was_pressed) // Only trigger on key press, not hold
        {
            player2->is_attacking = true;
            player2->attack_start_time = SDL_GetTicks();
            player2->current_attack = (player2->current_attack + 1) % 3; // Cycle through 0, 1, 2
            set_player2_state(player2, PLAYER2_ATTACKING);
            up_was_pressed = true;
        }
    }
    else
    {
        up_was_pressed = false;
    }

    // Block with down arrow - only block while key is held
    if (keystate[SDL_SCANCODE_DOWN] && !player2->is_attacking)
    {
        if (!player2->is_blocking)
        {
            player2->is_blocking = true;
            set_player2_state(player2, PLAYER2_BLOCKING);
        }
    }
    else
    {
        if (player2->is_blocking)
        {
            player2->is_blocking = false;
            set_player2_state(player2, PLAYER2_IDLE);
        }
    }

    // On air animation
    if (!player2->on_ground && player2->state != PLAYER2_JUMPING && !player2->is_attacking && !player2->is_blocking)
    {
        set_player2_state(player2, PLAYER2_JUMPING);
    }
}

void update_player2(Player2 *player2, Uint32 delta_time)
{
    if (!player2)
        return;

    float dt = delta_time / 1000.0f; // Convert to seconds

    // Update position
    player2->x += player2->velocity_x * dt;
    player2->y += player2->velocity_y * dt;

    // Apply gravity
    if (!player2->on_ground)
    {
        player2->velocity_y += player2->gravity * dt;
    }

    // Check ground collision (simple ground check)
    if (player2->y >= GROUND_LEVEL)
    {
        player2->y = GROUND_LEVEL;
        player2->velocity_y = 0;

        if (!player2->on_ground) // Just landed
        {
            player2->on_ground = true;
            if (player2->state == PLAYER2_JUMPING)
            {
                set_player2_state(player2, PLAYER2_IDLE);
            }
        }
    }
    else
    {
        player2->on_ground = false;
    }

    // Update attack state
    if (player2->is_attacking)
    {
        Uint32 current_time = SDL_GetTicks();
        if (current_time - player2->attack_start_time >= player2->attack_duration)
        {
            player2->is_attacking = false;
            set_player2_state(player2, PLAYER2_IDLE);
        }
    }

    // Update animation
    Uint32 current_time = SDL_GetTicks();
    if (current_time - player2->last_frame_time >= player2->frame_delay)
    {
        if (player2->state == PLAYER2_DEATH)
        {
            /* Let the animation run once and then freeze                */
            if (player2->current_frame < player2->frame_count - 1)
            {
                player2->current_frame++;
                player2->src_rect.x = player2->current_frame * player2->frame_width;
            }
            /* nothing happens once we are on the last frame             */
        }
        else if (player2->state == PLAYER2_ATTACKING)
        {
            if (player2->current_frame < player2->frame_count - 1)
            {
                player2->current_frame++;
            }
            else
            {
                /* animation finished; if the combat code has already
                   set is_attacking = false, drop back to idle         */
                if (!player2->is_attacking)
                    set_player2_state(player2, PLAYER2_IDLE);
            }
            player2->src_rect.x = player2->current_frame * player2->frame_width;
        }
        else /* every other state keeps looping as usual                 */
        {
            player2->current_frame = (player2->current_frame + 1) % player2->frame_count;
            player2->src_rect.x = player2->current_frame * player2->frame_width;
        }

        player2->last_frame_time = current_time;
    }

    // Update destination rectangle
    player2->dest_rect.x = (int)player2->x;
    player2->dest_rect.y = (int)player2->y;
    player2->dest_rect.w = player2->frame_width;
    player2->dest_rect.h = player2->frame_height;

    // Keep player2 on screen
    if (player2->x < 0)
        player2->x = 0;
    if (player2->x > 1280 - player2->frame_width)
        player2->x = 1280 - player2->frame_width;
}

void render_player2(SDL_Renderer *renderer, Player2 *player2)
{
    if (!player2 || !renderer)
        return;

    SDL_Texture *current_texture = NULL;

    // Select appropriate texture based on state
    switch (player2->state)
    {
    case PLAYER2_IDLE:
        current_texture = player2->idle_texture;
        break;
    case PLAYER2_WALKING:
        current_texture = player2->walking_texture;
        break;
    case PLAYER2_JUMPING:
        current_texture = player2->jumping_texture;
        break;
    case PLAYER2_ATTACKING:
        // Select attack texture based on current attack cycle
        switch (player2->current_attack)
        {
        case 0:
            current_texture = player2->attack_texture;
            break;
        case 1:
            current_texture = player2->attack2_texture;
            break;
        case 2:
            current_texture = player2->attack3_texture;
            break;
        }
        break;
    case PLAYER2_BLOCKING:
        current_texture = player2->block_texture;
        break;
    case PLAYER2_DEATH:
        current_texture = player2->death_texture;
        break;
    case PLAYER2_HURT:
        current_texture = player2->hurt_texture;
        break;
    }

    if (current_texture == player2->death_texture)
    {
        if (player2->current_frame >= player2->frame_count - 1)
        {
            player2->current_frame = player2->frame_count - 1;
        } // last frame of death animation
    }

    if (!current_texture)
    {
        fprintf(stderr, "No texture available for current player2 state %d\n", player2->state);
        return;
    }

    // Render with flip based on direction
    SDL_RendererFlip flip = (player2->direction == LEFT) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    if (SDL_RenderCopyEx(renderer, current_texture, &player2->src_rect, &player2->dest_rect,
                         0, NULL, flip) != 0)
    {
        fprintf(stderr, "Failed to render player2: %s\n", SDL_GetError());
    }
}

void set_player2_state(Player2 *player2, Player2State new_state)
{
    if (!player2)
        return;

    // Only change state if it's different (except for attacking which can cycle)
    if (player2->state == new_state && new_state != PLAYER2_ATTACKING)
        return;

    player2->state = new_state;
    player2->current_frame = 0;                // Reset animation
    player2->last_frame_time = SDL_GetTicks(); // Reset animation timer

    SDL_Texture *texture = NULL;
    int texture_width, texture_height;

    // Set frame count and delay based on the new state, and update frame width
    switch (new_state)
    {
    case PLAYER2_IDLE:
        texture = player2->idle_texture;
        player2->frame_count = 1;
        player2->frame_delay = 150; // Slower for idle
        break;
    case PLAYER2_WALKING:
        texture = player2->walking_texture;
        player2->frame_count = 7;
        player2->frame_delay = 100;
        break;
    case PLAYER2_JUMPING:
        texture = player2->jumping_texture;
        player2->frame_count = 6;
        player2->frame_delay = 100;
        break;
    case PLAYER2_ATTACKING:
        // Set frame count based on which attack animation
        switch (player2->current_attack)
        {
        case 0:
            texture = player2->attack_texture;
            player2->frame_count = 5; // Attack 1 has 5 frames
            break;
        case 1:
            texture = player2->attack2_texture;
            player2->frame_count = 4; // Attack 2 has 4 frames
            break;
        case 2:
            texture = player2->attack3_texture;
            player2->frame_count = 4; // Attack 3 has 4 frames
            break;
        }
        player2->frame_delay = 80; // Faster animation for attacks
        break;
    case PLAYER2_BLOCKING:
        texture = player2->block_texture;
        player2->frame_count = 1; // Single frame for blocking
        player2->frame_delay = 100;
        break;
    case PLAYER2_DEATH:
        texture = player2->death_texture;
        player2->frame_count = 6; // Adjust based on your death animation frames
        player2->frame_delay = 150;
        break;
    case PLAYER2_HURT:
        texture = player2->hurt_texture;
        player2->frame_count = 2; // Adjust based on your hurt animation frames
        player2->frame_delay = 100;
        break;
    }

    // Update frame width based on the texture
    if (texture)
    {
        get_texture_dimensions(texture, &texture_width, &texture_height);
        player2->frame_width = texture_width / player2->frame_count;

        // Update rectangles with new dimensions
        player2->src_rect.w = player2->frame_width;
        player2->dest_rect.w = player2->frame_width;
    }

    // Reset source rectangle to first frame
    player2->src_rect.x = 0;
}