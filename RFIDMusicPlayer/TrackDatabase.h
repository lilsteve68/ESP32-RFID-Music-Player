#pragma once

#include <Arduino.h>
#include <SD.h>

class TrackDatabase
{
public:
    static bool begin();
    static String getTrack(const String& uid);
    static bool setMapping(const String& uid, const String& filename);
    static bool removeMapping(const String& uid);
    static String getMappingsJson();

private:
    static const char* MAP_FILE;
    static void loadDatabase();
    static void saveDatabase();
};