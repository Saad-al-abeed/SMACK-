
#ifndef SOUND_API_H
#define SOUND_API_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* Player SFX */
    SK_P_IDLE, SK_P_WALK, SK_P_JUMP,
    SK_P_ATK1, SK_P_ATK2, SK_P_ATK3,
    SK_P_BLOCK, SK_P_HURT, SK_P_DEATH,
    SK_P_SLIDE, SK_P_BLOCK_HURT, SK_P_PRAY, SK_P_DOWN_ATK,
    /* Player2 SFX */
    SK_P2_IDLE, SK_P2_WALK, SK_P2_JUMP,
    SK_P2_ATK1, SK_P2_ATK2, SK_P2_ATK3,
    SK_P2_BLOCK, SK_P2_HURT, SK_P2_DEATH,
    SK_P2_SLIDE, SK_P2_BLOCK_HURT, SK_P2_PRAY, SK_P2_DOWN_ATK,
    /* Enemy SFX */
    SK_E_IDLE, SK_E_WALK, SK_E_JUMP,
    SK_E_ATK1, SK_E_ATK2, SK_E_ATK3,
    SK_E_BLOCK, SK_E_HURT, SK_E_DEATH,
    SK_E_SLIDE, SK_E_BLOCK_HURT, SK_E_PRAY, SK_E_DOWN_ATK,
    /* UI */
    SK_UI_HOVER, SK_UI_CLICK,
    /* Music */
    SK_BG_INTRO, SK_BG_MAP1, SK_BG_MAP2, SK_BG_MAP3,
    SK__COUNT
} SoundKey;

/* system */
void sound_system_init(void);
void sound_system_shutdown(void);

/* runtime config */
void sound_set_path(int key, const char* path);

/* bgm */
void sound_music_play_intro(void);
void sound_music_play_map(int which);
void sound_music_stop(void);

/* ui */
void sound_ui_hover(void);
void sound_ui_click(void);

/* optional volume */
void sound_set_sfx_volume(int vol);
void sound_set_music_volume(int vol);

#ifdef __cplusplus
}
#endif

#endif /* SOUND_API_H */
