#include "background.h"
#include <stdio.h>
#include <stdlib.h>

extern void sound_ui_hover(void);
extern void sound_ui_click(void);
extern void sound_music_play_map(int which);
extern void sound_music_stop(void);

Background *create_background(SDL_Renderer *ren, const char *path,
                              int total_frames, Uint32 frame_delay)
{
    Background *bg = (Background *)malloc(sizeof(Background));
    if (!bg)
    {
        fprintf(stderr, "Failed to allocate background\n");
        return NULL;
    }

    bg->texture = IMG_LoadTexture(ren, path);
    if (!bg->texture)
    {
        fprintf(stderr, "IMG_LoadTexture Error: %s\n", IMG_GetError());
        free(bg);
        return NULL;
    }

    // Get full texture dimensions
    int tex_width, tex_height;
    SDL_QueryTexture(bg->texture, NULL, NULL, &tex_width, &tex_height);

    // Calculate frame dimensions
    bg->frame_width = tex_width / total_frames;
    bg->frame_height = tex_height;
    bg->total_frames = total_frames;
    bg->current_frame = 0;
    bg->frame_delay = frame_delay;
    bg->last_update = SDL_GetTicks();

    return bg;
}

void update_background(Background *bg)
{
    if (!bg)
        return;

    Uint32 current_time = SDL_GetTicks();
    if (current_time - bg->last_update > bg->frame_delay)
    {
        bg->current_frame = (bg->current_frame + 1) % bg->total_frames;
        bg->last_update = current_time;
    }
}

void render_background(SDL_Renderer *ren, Background *bg)
{
    if (!bg || !bg->texture)
        return;

    // Create source rectangle for current frame
    SDL_Rect src_rect = {
        .x = bg->current_frame * bg->frame_width,
        .y = 0,
        .w = bg->frame_width,
        .h = bg->frame_height};

    // Get ren dimensions for full-screen background
    int render_w, render_h;
    SDL_GetRendererOutputSize(ren, &render_w, &render_h);

    // Create destination rectangle to fill the screen
    SDL_Rect dest_rect = {
        .x = 0,
        .y = 0,
        .w = render_w,
        .h = render_h};

    SDL_RenderCopy(ren, bg->texture, &src_rect, &dest_rect);
}

void destroy_background(Background *bg)
{
    if (bg)
    {
        if (bg->texture)
        {
            SDL_DestroyTexture(bg->texture);
        }
        free(bg);
    }
}

// Function to create a button with auto-sizing based on image dimensions
Button *create_button(SDL_Renderer *ren, int x, int y,
                      const char *normal_bmp, const char *hover_bmp)
{
    Button *button = (Button *)malloc(sizeof(Button));
    if (!button)
    {
        fprintf(stderr, "Failed to allocate button\n");
        return NULL;
    }

    button->rect.x = x;
    button->rect.y = y;
    button->normal_texture = IMG_LoadTexture(ren, normal_bmp);
    button->hover_texture = IMG_LoadTexture(ren, hover_bmp);
    button->is_hovered = false;
    button->is_pressed = false;

    // Get texture dimensions for auto-sizing
    if (button->normal_texture)
    {
        SDL_QueryTexture(button->normal_texture, NULL, NULL, &button->rect.w, &button->rect.h);
    }
    else
    {
        button->rect.w = 100; // Default width
        button->rect.h = 50;  // Default height
    }

    return button;
}

// Function to check if point is inside button
bool point_in_button(Button *button, int x, int y)
{
    if (!button)
        return false;

    return (x >= button->rect.x && x <= button->rect.x + button->rect.w &&
            y >= button->rect.y && y <= button->rect.y + button->rect.h);
}

// Function to handle button events
void handle_button_event(Button *button, SDL_Event *event)
{
    if (!button)
        return;

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    button->is_hovered = point_in_button(button, mouse_x, mouse_y);

    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
    {
        if (button->is_hovered)
        {
            button->is_pressed = true;
        }
    }
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        button->is_pressed = false;
    }
}

// Function to render button
void render_button(SDL_Renderer *ren, Button *button)
{
    if (!button)
        return;

    // Choose texture based on hover state
    SDL_Texture *current_texture = button->is_hovered ? button->hover_texture : button->normal_texture;

    // Render button image
    if (current_texture)
    {
        SDL_RenderCopy(ren, current_texture, NULL, &button->rect);
    }
    else
    {
        // Fallback: render a simple rectangle if texture loading failed
        SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
        SDL_RenderFillRect(ren, &button->rect);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderDrawRect(ren, &button->rect);
    }
}

// Function to check if button was clicked
bool play_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}

bool single_play_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}
bool multi_play_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}
bool map1_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}
bool map2_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}
bool map3_button_clicked(Button *button)
{
    if (!button)
        return false;

    return button->is_pressed && button->is_hovered;
}

// Function to cleanup button textures
void destroy_button(Button *button)
{
    if (button)
    {
        if (button->normal_texture)
        {
            SDL_DestroyTexture(button->normal_texture);
            button->normal_texture = NULL;
        }
        if (button->hover_texture)
        {
            SDL_DestroyTexture(button->hover_texture);
            button->hover_texture = NULL;
        }
        free(button);
    }
}