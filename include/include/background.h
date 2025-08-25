#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

typedef struct
{
    SDL_Texture *texture;
    int frame_width;    
    int frame_height;   
    int current_frame;  
    int total_frames;   
    Uint32 last_update; 
    Uint32 frame_delay; 
} Background;

typedef struct
{
    SDL_Rect rect;
    SDL_Texture *normal_texture;
    SDL_Texture *hover_texture;
    bool is_hovered;
    bool is_pressed;
} Button;

Background *create_background(SDL_Renderer *ren, const char *path,
                              int total_frames, Uint32 frame_delay);
void update_background(Background *bg);
void render_background(SDL_Renderer *ren, Background *bg);
void destroy_background(Background *bg);

Button *create_button(SDL_Renderer *renderer, int x, int y,
                              const char *normal_bmp, const char *hover_bmp);
bool point_in_button(Button *button, int x, int y);
void handle_button_event(Button *button, SDL_Event *event);
void render_button(SDL_Renderer *renderer, Button *button);
bool play_button_clicked(Button *button);
bool single_play_button_clicked(Button *button);
bool multi_play_button_clicked(Button *button);
bool map1_button_clicked(Button *button);
bool map2_button_clicked(Button *button);
bool map3_button_clicked(Button *button);
void destroy_button(Button *button);

#endif