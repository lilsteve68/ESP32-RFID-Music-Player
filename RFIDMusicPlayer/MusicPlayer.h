#pragma once

#include <Arduino.h>

#include "AudioTools.h"
#include "AudioTools/Disk/AudioSourceSD.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

#include "Pins.h"

class MusicPlayer
{
public:
    MusicPlayer();

    bool begin();

    bool play(const char *filename);

    void stop();

    void reset();

    void loop();

    bool isPlaying() const;

    // --- New Volume Methods ---
    void setVolume(float vol); // Accept 0.0f to 1.0f
    float getVolume() const;

private:
    AudioSourceSD *source = nullptr;
    I2SStream i2s;
    MP3DecoderHelix decoder;
    audio_tools::AudioPlayer *player = nullptr;

    bool playing = false;
    float currentVolume = 1.0f; // Default 100% volume
    String currentFile;
    
    void setMotorState(bool enable);
    void destroyPlayer();
    bool createPlayer();
};

extern MusicPlayer music;