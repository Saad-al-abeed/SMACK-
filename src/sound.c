#include "sound.h"
#include <string.h>
#include <stdio.h>

// --- Sound Storage ---
// Music
static Mix_Music *music_map1 = NULL;
static Mix_Music *music_map2 = NULL;
static Mix_Music *music_map3 = NULL;
static Mix_Music *menu_music = NULL;

// Sound Effects
static Mix_Chunk *sfx_attack = NULL;
static Mix_Chunk *sfx_jump = NULL;
static Mix_Chunk *sfx_hurt = NULL;
static Mix_Chunk *sfx_death = NULL;
static Mix_Chunk *sfx_struck = NULL;

// --- Helper Functions ---
static Mix_Music* load_music(const char* path) {
    Mix_Music* music = Mix_LoadMUS(path);
    if (!music) {
        fprintf(stderr, "Failed to load music %s! Mix_Error: %s\n", path, Mix_GetError());
    }
    return music;
}

static Mix_Chunk* load_sfx(const char* path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (!chunk) {
        fprintf(stderr, "Failed to load SFX %s! Mix_Error: %s\n", path, Mix_GetError());
    }
    return chunk;
}


// --- Public API Implementation ---

void sound_init(void) {
    // --- Load all your sounds here ---
    // You just need to provide the correct paths to your files.

    // Music
    menu_music = load_music("assets/sounds/assets_sounds_fire.mp3"); // Example path
    music_map1 = load_music("assets/sounds/assets_sounds_ambient.mp3"); // Example path
    music_map2 = load_music("assets/sounds/assets_sounds_ambient.mp3"); // Example path
    music_map3 = load_music("assets/sounds/assets_sounds_ambient.mp3"); // Example path

    // Sound Effects
    sfx_attack = load_sfx("assets/sounds/attack.wav");     // Example path
    sfx_jump = load_sfx("assets/sounds/jmp.wav");         // Example path
    sfx_hurt = load_sfx("assets/sounds/assets_sounds_lighthurt.wav");         // Example path
    sfx_death = load_sfx("assets/sounds/assets_sounds_death.wav");       // Example path
    sfx_struck = load_sfx("assets/sounds/assets_sounds_sword4.wav");
}

void sound_play_music(const char* map_name) {
    Mix_Music* music_to_play = NULL;

    if (strcmp(map_name, "menu") == 0) music_to_play = menu_music;
    if (strcmp(map_name, "map1") == 0) music_to_play = music_map1;
    if (strcmp(map_name, "map2") == 0) music_to_play = music_map2;
    if (strcmp(map_name, "map3") == 0) music_to_play = music_map3;

    if (music_to_play) {
        Mix_FadeInMusic(music_to_play, -1, 2000); // Play music with a 2-second fade-in
    }
}

void sound_play_effects(Player *p1, Player2 *p2, Enemy *enemy) {
    // We use static bools to track the state from the previous frame.
    // This ensures a sound effect only plays ONCE when the state changes.
    static PlayerState last_p1_state = PLAYER_IDLE;
    static Player2State last_p2_state = PLAYER2_IDLE;
    static EnemyState last_enemy_state = ENEMY_IDLE;

    // --- Player 1 Sound Effects ---
    if (p1) {
        if ((p1->state == PLAYER_ATTACKING && last_p1_state != PLAYER_ATTACKING) || (p1->state == PLAYER_DOWN_ATTACK && last_p1_state != PLAYER_DOWN_ATTACK)) Mix_PlayChannel(-1, sfx_attack, 0);
        if (p1->state == PLAYER_JUMPING && last_p1_state != PLAYER_JUMPING) Mix_PlayChannel(-1, sfx_jump, 0);
        if ((p1->state == PLAYER_HURT && last_p1_state != PLAYER_HURT) || (p1->state == PLAYER_BLOCK_HURT && last_p1_state != PLAYER_BLOCK_HURT)) {
            Mix_PlayChannel(-1, sfx_hurt, 0);
            Mix_PlayChannel(-1, sfx_struck, 0);
        }
        if (p1->state == PLAYER_DEATH && last_p1_state != PLAYER_DEATH) Mix_PlayChannel(-1, sfx_death, 0);
        last_p1_state = p1->state;
    }

    // --- Player 2 Sound Effects ---
    if (p2) {
        if ((p2->state == PLAYER2_ATTACKING && last_p2_state != PLAYER2_ATTACKING) || (p2->state == PLAYER2_DOWN_ATTACK && last_p2_state != PLAYER2_DOWN_ATTACK)) Mix_PlayChannel(-1, sfx_attack, 0);
        if (p2->state == PLAYER2_JUMPING && last_p2_state != PLAYER2_JUMPING) Mix_PlayChannel(-1, sfx_jump, 0);
        if ((p2->state == PLAYER2_HURT && last_p2_state != PLAYER2_HURT) || (p2->state == PLAYER2_BLOCK_HURT && last_p2_state != PLAYER2_BLOCK_HURT)) {
            Mix_PlayChannel(-1, sfx_hurt, 0);
            Mix_PlayChannel(-1, sfx_struck, 0);
        }
        if (p2->state == PLAYER2_DEATH && last_p2_state != PLAYER2_DEATH) Mix_PlayChannel(-1, sfx_death, 0);
        last_p2_state = p2->state;
    }

    // --- Enemy Sound Effects ---
    if (enemy) {
        if (enemy->state == ENEMY_ATTACKING && last_enemy_state != ENEMY_ATTACKING) Mix_PlayChannel(-1, sfx_attack, 0);
        if (enemy->state == ENEMY_JUMPING && last_enemy_state != ENEMY_JUMPING) Mix_PlayChannel(-1, sfx_jump, 0);
        if ((enemy->state == ENEMY_HURT && last_enemy_state != ENEMY_HURT) || (enemy->state == ENEMY_BLOCK_HURT && last_enemy_state != ENEMY_BLOCK_HURT)){
            Mix_PlayChannel(-1, sfx_hurt, 0);
            Mix_PlayChannel(-1, sfx_struck, 0);
        }
        if (enemy->state == ENEMY_DEATH && last_enemy_state != ENEMY_DEATH) Mix_PlayChannel(-1, sfx_death, 0);
        last_enemy_state = enemy->state;
    }
}

void sound_stop_all(void) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1); // Stop all sound effects
}

void sound_quit(void) {
    // Free all loaded sound resources
    Mix_FreeMusic(menu_music);
    Mix_FreeMusic(music_map1);
    Mix_FreeMusic(music_map2);
    Mix_FreeMusic(music_map3);
    
    Mix_FreeChunk(sfx_attack);
    Mix_FreeChunk(sfx_jump);
    Mix_FreeChunk(sfx_hurt);
    Mix_FreeChunk(sfx_death);
}