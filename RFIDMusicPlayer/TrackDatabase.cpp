#include "TrackDatabase.h"
#include "Pins.h"

const char* TrackDatabase::MAP_FILE = "/mappings.json";

// In-memory key-value cache
struct Mapping {
    String uid;
    String file;
};

#define MAX_MAPPINGS 50
static Mapping mappingList[MAX_MAPPINGS];
static int mappingCount = 0;

bool TrackDatabase::begin()
{
    loadDatabase();
    return true;
}

String TrackDatabase::getTrack(const String& uid)
{
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappingList[i].uid.equalsIgnoreCase(uid))
        {
            return mappingList[i].file;
        }
    }
    return "";
}

bool TrackDatabase::setMapping(const String& uid, const String& filename)
{
    if (uid.length() == 0 || filename.length() == 0) return false;

    // Update existing mapping if UID exists
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappingList[i].uid.equalsIgnoreCase(uid))
        {
            mappingList[i].file = filename;
            saveDatabase();
            return true;
        }
    }

    // Add new mapping if space is available
    if (mappingCount < MAX_MAPPINGS)
    {
        mappingList[mappingCount].uid = uid;
        mappingList[mappingCount].file = filename;
        mappingCount++;
        saveDatabase();
        return true;
    }

    return false;
}

bool TrackDatabase::removeMapping(const String& uid)
{
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappingList[i].uid.equalsIgnoreCase(uid))
        {
            for (int j = i; j < mappingCount - 1; j++)
            {
                mappingList[j] = mappingList[j + 1];
            }
            mappingCount--;
            saveDatabase();
            return true;
        }
    }
    return false;
}

String TrackDatabase::getMappingsJson()
{
    String json = "{";
    for (int i = 0; i < mappingCount; i++)
    {
        json += "\"" + mappingList[i].uid + "\":\"" + mappingList[i].file + "\"";
        if (i < mappingCount - 1) json += ",";
    }
    json += "}";
    return json;
}

void TrackDatabase::loadDatabase()
{
    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, LOW);

    if (!SD.exists(MAP_FILE))
    {
        digitalWrite(SD_CS, HIGH);
        return;
    }

    File file = SD.open(MAP_FILE, FILE_READ);
    if (!file)
    {
        digitalWrite(SD_CS, HIGH);
        return;
    }

    String content = file.readString();
    file.close();
    digitalWrite(SD_CS, HIGH);

    mappingCount = 0;

    // Parsing JSON pairs {"UID":"FILE"}
    int pos = 0;
    while ((pos = content.indexOf('"', pos)) != -1 && mappingCount < MAX_MAPPINGS)
    {
        int keyEnd = content.indexOf('"', pos + 1);
        if (keyEnd == -1) break;
        String key = content.substring(pos + 1, keyEnd);

        int valStart = content.indexOf('"', keyEnd + 1);
        if (valStart == -1) break;
        int valEnd = content.indexOf('"', valStart + 1);
        if (valEnd == -1) break;
        String val = content.substring(valStart + 1, valEnd);

        mappingList[mappingCount].uid = key;
        mappingList[mappingCount].file = val;
        mappingCount++;

        pos = valEnd + 1;
    }
}

void TrackDatabase::saveDatabase()
{
    digitalWrite(RFID_CS, HIGH);
    digitalWrite(SD_CS, LOW);

    File file = SD.open(MAP_FILE, FILE_WRITE);
    if (file)
    {
        file.print(getMappingsJson());
        file.close();
        Serial.println("[DB] Mappings saved to SD card.");
    }
    else
    {
        Serial.println("[ERROR] Failed to write mappings.json");
    }

    digitalWrite(SD_CS, HIGH);
}