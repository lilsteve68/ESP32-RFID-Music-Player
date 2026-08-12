#include "Storage.h"

bool Storage::begin()
{
    digitalWrite(RFID_CS, HIGH);   // Disable RC522
    digitalWrite(SD_CS, LOW);      // Select SD

    if (!sd.begin(SD_CS, SD_SCK_MHZ(4)))
    {
        digitalWrite(SD_CS, HIGH);
        return false;
    }

    digitalWrite(SD_CS, HIGH);

    return true;
}

void Storage::listRoot()
{
    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, LOW);

    FsFile root = sd.open("/");

    FsFile file;

    while (file.openNext(&root, O_RDONLY))
    {
        char name[64];

        file.getName(name, sizeof(name));

        Serial.println(name);

        file.close();
    }

    root.close();

    digitalWrite(SD_CS, HIGH);
}