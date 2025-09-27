#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include "player.h" // For player states
#include "player2.h" // For player2 states
#include "enemy.h"   // For enemy states


void sound_init(void);

void sound_play_music(const char* map_name);

void sound_play_effects(Player *p1, Player2 *p2, Enemy *enemy);

void sound_stop_all(void);

void sound_quit(void);

#endif // SOUND_H