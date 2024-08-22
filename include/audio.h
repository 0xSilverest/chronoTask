#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

int initialize_audio();
void play_notification_sound();
void cleanup_audio();

#endif
