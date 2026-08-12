#pragma once

#include <Arduino.h>
#include <SdFat.h>

#include "Pins.h"

class Storage
{
public:
    bool begin();
    void listRoot();

private:
    SdFat sd;
};