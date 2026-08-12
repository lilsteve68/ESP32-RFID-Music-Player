#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Pins.h"

class RFID
{
public:
    RFID();

    bool begin();
    void update();
    bool available();
    String getUID() const;
    bool isPresent() const;
    void reset();

private:
    MFRC522 mfrc522;

    String uid;

    bool present = false;
    bool newTag = false;

    unsigned long lastSeen = 0;

    static const uint32_t TAG_TIMEOUT = 1200; // ms

    bool checkTagPresent();
};

extern RFID rfid;