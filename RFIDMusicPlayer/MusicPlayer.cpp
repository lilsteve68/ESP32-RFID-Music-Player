#include "MusicPlayer.h"
#include <SD.h>

MusicPlayer music;

MusicPlayer::MusicPlayer() {}

bool MusicPlayer::begin()
{
    Serial.println("[AUDIO] Initializing...");

    // Setup Motor Control Pin
    pinMode(MOTOR_PIN, OUTPUT);
    setMotorState(false); // Off by default

    if (!SD.begin(SD_CS))
    {
        Serial.println("[AUDIO] SD Card Mount Failed");
        return false;
    }

    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck  = I2S_BCLK;
    cfg.pin_ws   = I2S_LRC;
    cfg.pin_data = I2S_DOUT;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;

    if (!i2s.begin(cfg))
    {
        Serial.println("[AUDIO] I2S init failed");
        return false;
    }

    source = new AudioSourceSD("/", "mp3", SD_CS);
    player = new audio_tools::AudioPlayer(*source, i2s, decoder);
    player->setVolume(currentVolume);
    player->begin();

    Serial.println("[AUDIO] Ready");
    return true;
}

void MusicPlayer::setVolume(float vol)
{
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;

    currentVolume = vol;
    if (player)
    {
        player->setVolume(currentVolume);
    }
    Serial.print("[AUDIO] Volume set to: ");
    Serial.print((int)(currentVolume * 100));
    Serial.println("%");
}

float MusicPlayer::getVolume() const
{
    return currentVolume;
}

bool MusicPlayer::play(const char *filename)
{
    Serial.print("[AUDIO] Play: ");
    Serial.println(filename);

    digitalWrite(RFID_CS, HIGH); 
    digitalWrite(SD_CS, LOW);

    bool fileExists = SD.exists(filename);

    digitalWrite(SD_CS, HIGH);

    if (!fileExists)
    {
        Serial.println("[AUDIO] File not found on SD card");
        setMotorState(false);
        playing = false;
        return false;
    }

    currentFile = filename;
    player->setPath(filename);
    player->setActive(true);
    player->setVolume(currentVolume); // Re-apply current volume on new track
    playing = true;

    setMotorState(true);
    return true;
}

void MusicPlayer::stop()
{
    if (!player || !playing) return;

    Serial.println("[AUDIO] Stop");
    player->setActive(false);
    playing = false;

    i2s.flush();
    setMotorState(false);
}

void MusicPlayer::reset()
{
    stop();
    
    decoder.end();
    decoder.begin();

    currentFile = "";
}

void MusicPlayer::loop()
{
    if (player && playing)
    {
        if (!player->copy())
        {
            Serial.println("[AUDIO] Track finished.");
            stop();
        }
    }
}

bool MusicPlayer::isPlaying() const
{
    return playing;
}

void MusicPlayer::setMotorState(bool enable)
{
    digitalWrite(MOTOR_PIN, enable ? HIGH : LOW);
    Serial.print("[MOTOR] State set to: ");
    Serial.println(enable ? "HIGH" : "LOW");
}