
/* sound.c — centralized game audio for animations, UI, and backgrounds */
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>

/* ----- Keys to set paths at runtime ----- */
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

static inline int key_is_music(int k) {
    return (k==SK_BG_INTRO||k==SK_BG_MAP1||k==SK_BG_MAP2||k==SK_BG_MAP3);
}

#define MAX_PATH_LEN 512
static char g_paths[SK__COUNT][MAX_PATH_LEN];
static Mix_Chunk* g_sfx[SK__COUNT];
static Mix_Music* g_music[SK__COUNT];

static Mix_Chunk* load_chunk_path(const char* p){
    if(!p||!*p) return NULL;
    Mix_Chunk* c = Mix_LoadWAV(p);
    if(!c) fprintf(stderr,"[sound] Mix_LoadWAV %s failed: %s\n",p,Mix_GetError());
    return c;
}
static Mix_Music* load_music_path(const char* p){
    if(!p||!*p) return NULL;
    Mix_Music* m = Mix_LoadMUS(p);
    if(!m) fprintf(stderr,"[sound] Mix_LoadMUS %s failed: %s\n",p,Mix_GetError());
    return m;
}
static void reload_key(int k){
    if(key_is_music(k)){
        if(g_music[k]){ Mix_FreeMusic(g_music[k]); g_music[k]=NULL; }
        g_music[k]=load_music_path(g_paths[k]);
    }else{
        if(g_sfx[k]){ Mix_FreeChunk(g_sfx[k]); g_sfx[k]=NULL; }
        g_sfx[k]=load_chunk_path(g_paths[k]);
    }
}
static void play_sfx_key(int k){
    if(k<0||k>=SK__COUNT||key_is_music(k)) return;
    if(g_sfx[k]) Mix_PlayChannel(-1,g_sfx[k],0);
}
static void play_music_key(int k){
    if(k<0||k>=SK__COUNT||!key_is_music(k)) return;
    if(!g_music[k]) return;
    if(Mix_PlayingMusic()) Mix_HaltMusic();
    Mix_PlayMusic(g_music[k], -1);
}

/* Public API */
void sound_system_init(void){
    for(int i=0;i<SK__COUNT;++i){ g_paths[i][0]='\0'; g_sfx[i]=NULL; g_music[i]=NULL; }
}
void sound_system_shutdown(void){
    Mix_HaltMusic();
    for(int i=0;i<SK__COUNT;++i){
        if(g_sfx[i]){ Mix_FreeChunk(g_sfx[i]); g_sfx[i]=NULL; }
        if(g_music[i]){ Mix_FreeMusic(g_music[i]); g_music[i]=NULL; }
    }
}
void sound_set_path(int key, const char* path){
    if(key<0||key>=SK__COUNT) return;
    if(!path) path="";
    strncpy(g_paths[key], path, MAX_PATH_LEN-1);
    g_paths[key][MAX_PATH_LEN-1]='\0';
    reload_key(key);
}

/* Background & UI helpers */
void sound_music_play_intro(void){ play_music_key(SK_BG_INTRO); }
void sound_music_play_map(int which){
    switch(which){
        case 1: play_music_key(SK_BG_MAP1); break;
        case 2: play_music_key(SK_BG_MAP2); break;
        case 3: play_music_key(SK_BG_MAP3); break;
        default: break;
    }
}
void sound_music_stop(void){ Mix_HaltMusic(); }
void sound_ui_hover(void){ play_sfx_key(SK_UI_HOVER); }
void sound_ui_click(void){ play_sfx_key(SK_UI_CLICK); }

/* Thin wrappers to keep other files enum-aware without headers */
void sound__p_idle(void){ play_sfx_key(SK_P_IDLE); }
void sound__p_walk(void){ play_sfx_key(SK_P_WALK); }
void sound__p_jump(void){ play_sfx_key(SK_P_JUMP); }
void sound__p_atk(int idx){ if(idx==1) play_sfx_key(SK_P_ATK2); else if(idx==2) play_sfx_key(SK_P_ATK3); else play_sfx_key(SK_P_ATK1); }
void sound__p_block(void){ play_sfx_key(SK_P_BLOCK); }
void sound__p_hurt(void){ play_sfx_key(SK_P_HURT); }
void sound__p_death(void){ play_sfx_key(SK_P_DEATH); }
void sound__p_slide(void){ play_sfx_key(SK_P_SLIDE); }
void sound__p_block_hurt(void){ play_sfx_key(SK_P_BLOCK_HURT); }
void sound__p_pray(void){ play_sfx_key(SK_P_PRAY); }
void sound__p_down_attack(void){ play_sfx_key(SK_P_DOWN_ATK); }

void sound__p2_idle(void){ play_sfx_key(SK_P2_IDLE); }
void sound__p2_walk(void){ play_sfx_key(SK_P2_WALK); }
void sound__p2_jump(void){ play_sfx_key(SK_P2_JUMP); }
void sound__p2_atk(int idx){ if(idx==1) play_sfx_key(SK_P2_ATK2); else if(idx==2) play_sfx_key(SK_P2_ATK3); else play_sfx_key(SK_P2_ATK1); }
void sound__p2_block(void){ play_sfx_key(SK_P2_BLOCK); }
void sound__p2_hurt(void){ play_sfx_key(SK_P2_HURT); }
void sound__p2_death(void){ play_sfx_key(SK_P2_DEATH); }
void sound__p2_slide(void){ play_sfx_key(SK_P2_SLIDE); }
void sound__p2_block_hurt(void){ play_sfx_key(SK_P2_BLOCK_HURT); }
void sound__p2_pray(void){ play_sfx_key(SK_P2_PRAY); }
void sound__p2_down_attack(void){ play_sfx_key(SK_P2_DOWN_ATK); }

void sound__e_idle(void){ play_sfx_key(SK_E_IDLE); }
void sound__e_walk(void){ play_sfx_key(SK_E_WALK); }
void sound__e_jump(void){ play_sfx_key(SK_E_JUMP); }
void sound__e_atk(int idx){ if(idx==1) play_sfx_key(SK_E_ATK2); else if(idx==2) play_sfx_key(SK_E_ATK3); else play_sfx_key(SK_E_ATK1); }
void sound__e_block(void){ play_sfx_key(SK_E_BLOCK); }
void sound__e_hurt(void){ play_sfx_key(SK_E_HURT); }
void sound__e_death(void){ play_sfx_key(SK_E_DEATH); }
void sound__e_slide(void){ play_sfx_key(SK_E_SLIDE); }
void sound__e_block_hurt(void){ play_sfx_key(SK_E_BLOCK_HURT); }
void sound__e_pray(void){ play_sfx_key(SK_E_PRAY); }
void sound__e_down_attack(void){ play_sfx_key(SK_E_DOWN_ATK); }
