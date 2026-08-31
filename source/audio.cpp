#include "audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <mutex>
#include <map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
    int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output);
    extern int32_t CGsSound_GetSndIDFromListIdx(void *CGsSound_instance, int32_t idx);
}

extern void *g_CGsSound_instance;

static ALCdevice *gDevice = nullptr;
static ALCcontext *gContext = nullptr;

static std::map<int, ALuint> gAudioCache;
static std::vector<ALuint> gActiveSfxSources;
static std::mutex gAudioMutex;
static ALuint gMusicSource = 0;

void audio_init() {
    gDevice = alcOpenDevice(nullptr);
    if (gDevice) {
        gContext = alcCreateContext(gDevice, nullptr);
        alcMakeContextCurrent(gContext);
    }
    
    alGenSources(1, &gMusicSource);
}

void audio_cleanup() {
    std::lock_guard<std::mutex> lock(gAudioMutex);
    alcMakeContextCurrent(gContext);

    if (gMusicSource) {
        alSourceStop(gMusicSource);
        alDeleteSources(1, &gMusicSource);
        gMusicSource = 0;
    }

    for (ALuint source : gActiveSfxSources) {
        alSourceStop(source);
        alDeleteSources(1, &source);
    }
    gActiveSfxSources.clear();

    for (auto const& [id, buffer] : gAudioCache) {
        alDeleteBuffers(1, &buffer);
    }
    gAudioCache.clear();

    if (gContext) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(gContext);
        gContext = nullptr;
    }
    if (gDevice) {
        alcCloseDevice(gDevice);
        gDevice = nullptr;
    }
}

void audio_play_sound(int sndID, int vol, int isLoop) {
    std::lock_guard<std::mutex> lock(gAudioMutex);

    int xSndId = CGsSound_GetSndIDFromListIdx(g_CGsSound_instance, sndID);
    
    if (gContext) {
        alcMakeContextCurrent(gContext);
    }

    if (gAudioCache.find(xSndId) == gAudioCache.end()) {
        char filepath[256];
        snprintf(filepath, sizeof(filepath), DATA_PATH "/res/raw/s%03d.ogg", xSndId);

        short *data = nullptr;
        int channels = 0, sample_rate = 0;
        int num_samples = stb_vorbis_decode_filename(filepath, &channels, &sample_rate, &data);

        if (num_samples > 0 && data != nullptr) {
            ALenum format = (channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
            ALuint buffer = 0;
            alGenBuffers(1, &buffer);
            alBufferData(buffer, format, data, num_samples * channels * sizeof(short), sample_rate);
            free(data);

            gAudioCache[xSndId] = buffer;
        } else {
            return; 
        }
    }

    ALuint buffer = gAudioCache[xSndId];
    float volume = (float)vol / 100.0f;

    if (xSndId >= 100) {
        if (gMusicSource) {
            ALint state;
            alGetSourcei(gMusicSource, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING || state == AL_PAUSED) {
                alSourceStop(gMusicSource);
            }
            // Safely detach the old buffer before assigning a new one
            alSourcei(gMusicSource, AL_BUFFER, 0);
        }
        alSourcei(gMusicSource, AL_BUFFER, buffer);
        alSourcef(gMusicSource, AL_GAIN, volume);
        alSourcei(gMusicSource, AL_LOOPING, isLoop ? AL_TRUE : AL_FALSE);
        alSourcePlay(gMusicSource);
    } else {
        for (auto it = gActiveSfxSources.begin(); it != gActiveSfxSources.end(); ) {
            ALint state;
            alGetSourcei(*it, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING) {
                alDeleteSources(1, &(*it));
                it = gActiveSfxSources.erase(it);
            } else {
                ++it;
            }
        }

        ALuint sfxSource = 0;
        alGenSources(1, &sfxSource);
        if (sfxSource) {
            alSourcei(sfxSource, AL_BUFFER, buffer);
            alSourcef(sfxSource, AL_GAIN, volume);
            alSourcei(sfxSource, AL_LOOPING, isLoop ? AL_TRUE : AL_FALSE);
            alSourcePlay(sfxSource);
            gActiveSfxSources.push_back(sfxSource);
        }
    }
}

void audio_stop_sound() {
    std::lock_guard<std::mutex> lock(gAudioMutex);
    if (gContext) {
        alcMakeContextCurrent(gContext);
    }
    
    if (gMusicSource) {
        alSourceStop(gMusicSource);
        alSourcei(gMusicSource, AL_BUFFER, 0);
    }

    for (ALuint source : gActiveSfxSources) {
        alSourceStop(source);
        alDeleteSources(1, &source);
    }
    gActiveSfxSources.clear();
}