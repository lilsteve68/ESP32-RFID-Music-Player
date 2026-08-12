#include "RFID.h"

RFID rfid;

RFID::RFID() : mfrc522(RFID_CS, RFID_RST) {}

bool RFID::begin()
{
    pinMode(RFID_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);
    pinMode(RFID_RST, OUTPUT);

    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, HIGH);

    digitalWrite(RFID_RST, LOW);
    delay(20);
    digitalWrite(RFID_RST, HIGH);
    delay(50);

    mfrc522.PCD_Init();
    delay(50);

    mfrc522.PCD_SetAntennaGain(MFRC522::RxGain_max);
    mfrc522.PCD_AntennaOn();

    digitalWrite(SD_CS, HIGH);
    digitalWrite(RFID_CS, LOW);
    byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    digitalWrite(RFID_CS, HIGH);

    Serial.print("[RFID] Version: 0x");
    Serial.println(version, HEX);

    if (version == 0x00 || version == 0xFF)
    {
        Serial.println("[RFID] RC522 not responding");
        return false;
    }

    return true;
}

void RFID::update()
{
    // 1. If no tag is currently marked as present, look for a new one
    if (!present)
    {
        digitalWrite(SD_CS, HIGH);
        digitalWrite(RFID_CS, LOW);

        // Clear FIFO buffer before reading
        mfrc522.PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x80);

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            uid = "";
            for (byte i = 0; i < mfrc522.uid.size; i++)
            {
                if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
                uid += String(mfrc522.uid.uidByte[i], HEX);
            }
            uid.toUpperCase();

            present = true;
            newTag = true;
            lastSeen = millis();

            Serial.print("[RFID] Tag Placed: ");
            Serial.println(uid);
        }

        digitalWrite(RFID_CS, HIGH);
        return;
    }

    // 2. If tag is marked present, check if it's still physically on the reader
    if (checkTagPresent())
    {
        lastSeen = millis();
    }

    // 3. Force timeout check: Mark removed if tag hasn't responded for > 300ms
    if (millis() - lastSeen > 300)
    {
        Serial.println("[RFID] Tag removed");
        present = false;
        newTag = false;
        uid = "";
    }
}

bool RFID::checkTagPresent()
{
    digitalWrite(SD_CS, HIGH);
    digitalWrite(RFID_CS, LOW);

    // Prepare buffer and re-select active card UID
    mfrc522.PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x80);

    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);

    // Request A to see if RF field gets a response
    MFRC522::StatusCode result = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);

    if (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION)
    {
        // Re-authenticate card presence using stored UID
        MFRC522::StatusCode selectResult = mfrc522.PICC_Select(&mfrc522.uid, 0);
        digitalWrite(RFID_CS, HIGH);
        
        return (selectResult == MFRC522::STATUS_OK);
    }

    digitalWrite(RFID_CS, HIGH);
    return false;
}

bool RFID::available()
{
    if (newTag)
    {
        newTag = false;
        return true;
    }
    return false;
}

String RFID::getUID() const { return uid; }
bool RFID::isPresent() const { return present; }

void RFID::reset()
{
    uid = "";
    present = false;
    newTag = false;
    lastSeen = 0;
}