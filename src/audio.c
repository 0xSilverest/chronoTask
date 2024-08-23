#include "audio.h"
#include "error_report.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

Mix_Chunk *notification_sound = NULL;

int initialize_audio() {
    LOG_INFO("Initializing SDL audio subsystem");
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        LOG_ERROR("SDL could not initialize! SDL Error: %s", SDL_GetError());
        return 0;
    }
    LOG_DEBUG("SDL audio subsystem initialized successfully");

    LOG_INFO("Opening audio device");
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        LOG_ERROR("SDL_mixer could not initialize! SDL_mixer Error: %s", Mix_GetError());
        return 0;
    }
    LOG_DEBUG("Audio device opened successfully");

    LOG_INFO("Loading notification sound file");
    notification_sound = Mix_LoadWAV(config.notification_sound);
    if (notification_sound == NULL) {
        LOG_ERROR("Failed to load notification sound! SDL_mixer Error: %s", Mix_GetError());
        return 0;
    }
    LOG_DEBUG("Notification sound loaded successfully");

    LOG_INFO("Audio initialized successfully");
    return 1;
}

void play_notification_sound() {
    if (notification_sound != NULL) {
        if (Mix_PlayChannel(-1, notification_sound, 0) == -1) {
            LOG_WARNING("Failed to play notification sound! SDL_mixer Error: %s", Mix_GetError());
        }
    } else {
        LOG_WARNING("Attempted to play notification sound, but it's not loaded");
    }
}

void cleanup_audio() {
    LOG_INFO("Cleaning up audio resources");
    if (notification_sound != NULL) {
        Mix_FreeChunk(notification_sound);
        notification_sound = NULL;
    }
    Mix_CloseAudio();
    SDL_Quit();
    LOG_DEBUG("Audio resources cleaned up");
}
