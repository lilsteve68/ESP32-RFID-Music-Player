#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "esp_sleep.h"

#include "Pins.h"
#include "RFID.h"
#include "MusicPlayer.h"
#include "TrackDatabase.h"
#include "WebServerManager.h"

enum PlayerState
{
    WAITING_FOR_TAG,
    PLAYING_MUSIC,
    WAIT_TAG_REMOVAL,
    LOW_POWER_SLEEP
};

PlayerState state = WAITING_FOR_TAG;

// Timers
unsigned long lastActivity = 0;
const unsigned long IDLE_TIMEOUT = 2 * 60 * 1000; // 2 minutes before powering down

void wakeSystem()
{
    Serial.println("[POWER] Tag detected! Waking system up...");
    
    // Restore Wi-Fi & Web Server
    webServerManager.begin("RFID-MusicPlayer", "password123");
    
    lastActivity = millis();
}

void enterLowPowerMode()
{
    Serial.println("[POWER] Idle timeout reached. Disabling Wi-Fi & entering Light Sleep...");
    
    // Shut down Wi-Fi to save battery
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    music.reset();
    state = LOW_POWER_SLEEP;
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("\n====================");
    Serial.println(" RFID Music Linker  ");
    Serial.println("====================");

    pinMode(RFID_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, HIGH);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    if (!rfid.begin())
    {
        Serial.println("RFID FAILED");
        while (1);
    }

    if (!music.begin())
    {
        Serial.println("AUDIO FAILED");
        while (1);
    }

    TrackDatabase::begin();

    // Start Wi-Fi & Web Server on boot
    webServerManager.begin("RFID-MusicPlayer", "password123");

    lastActivity = millis();
    Serial.println("System Ready! Waiting for tags...");
}

void loop()
{
    // =========================================
    // Low Power / Sleep Handling
    // =========================================
    if (state == LOW_POWER_SLEEP)
    {
        // 1. Configure micro-sleep for 500ms
        esp_sleep_enable_timer_wakeup(100 * 1000); // 100,000 microseconds
        esp_light_sleep_start();                   // Halts CPU & drops current draw to ~2mA

        // 2. Wake up every 500ms briefly to poll the RFID reader over SPI
        rfid.update();

        // 3. Check if a tag was placed while we were polling
        if (rfid.available())
        {
            wakeSystem();
            state = WAITING_FOR_TAG;
            // Fall through to normal state machine processing below
        }
        else
        {
            return; // Back to light sleep loop
        }
    }

    // =========================================
    // Active System Handling
    // =========================================
    rfid.update();
    webServerManager.update();

    // Check for idle timeout
    if (state == WAITING_FOR_TAG && (millis() - lastActivity > IDLE_TIMEOUT))
    {
        enterLowPowerMode();
        return;
    }

    switch (state)
    {
        case WAITING_FOR_TAG:
        {
            if (rfid.available())
            {
                lastActivity = millis(); // Reset idle timer

                String uid = rfid.getUID();
                webServerManager.setLastUID(uid);

                Serial.print("Tag UID: ");
                Serial.println(uid);

                String file = TrackDatabase::getTrack(uid);

                if (file.length() > 0)
                {
                    Serial.print("Playing: ");
                    Serial.println(file);

                    if (music.play(file.c_str()))
                    {
                        state = PLAYING_MUSIC;
                    }
                    else
                    {
                        state = WAIT_TAG_REMOVAL;
                    }
                }
                else
                {
                    Serial.println("Unmapped tag. Link it on Web Dashboard!");
                    state = WAIT_TAG_REMOVAL;
                }
            }
            break;
        }

        case PLAYING_MUSIC:
        {
            lastActivity = millis(); // Keep timer reset while music plays

            if (!rfid.isPresent())
            {
                music.reset();
                rfid.reset();
                state = WAITING_FOR_TAG;
            }
            else if (!music.isPlaying())
            {
                state = WAIT_TAG_REMOVAL;
            }
            else
            {
                music.loop();
            }
            break;
        }

        case WAIT_TAG_REMOVAL:
        {
            lastActivity = millis(); // Keep timer reset while card rests on reader

            if (!rfid.isPresent())
            {
                rfid.reset();
                state = WAITING_FOR_TAG;
            }
            break;
        }

        case LOW_POWER_SLEEP:
            break;
    }
}