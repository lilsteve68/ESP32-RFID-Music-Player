#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebServerManager
{
public:
    void begin(const char* ssid, const char* password);
    void update();
    void setLastUID(const String& uid);

private:
    WebServer server{80};
    String lastUID = "None";

    void handleRoot();
    void handleStatus();
    void handleTracks();
    void handleMappings();
    void handleSaveMapping();
    void handleDeleteMapping();
    void handleControl();
    void handleVolume();
};

extern WebServerManager webServerManager;