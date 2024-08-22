#include "audio.h"
#include <stdio.h>

Mix_Chunk *notification_sound = NULL;

int initialize_audio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return 0;
    }

    notification_sound = Mix_LoadWAV("notification.wav");
    if (notification_sound == NULL) {
        fprintf(stderr, "Failed to load notification sound! SDL_mixer Error: %s\n", Mix_GetError());
        return 0;
    }

    return 1;
}

void play_notification_sound() {
    if (notification_sound != NULL) {
        Mix_PlayChannel(-1, notification_sound, 0);
    }
}

void cleanup_audio() {
    if (notification_sound != NULL) {
        Mix_FreeChunk(notification_sound);
    }
    Mix_CloseAudio();
    SDL_Quit();
}
