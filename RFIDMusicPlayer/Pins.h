#pragma once

//==========================
// SPI BUS
//==========================

#define SPI_SCK     18
#define SPI_MISO    19
#define SPI_MOSI    23

//==========================
// RC522
//==========================

#define RFID_CS     5
#define RFID_RST    22

//==========================
// SD CARD
//==========================

#define SD_CS       21

//==========================
// MAX98357A
//==========================

#define I2S_BCLK    27
#define I2S_LRC     25
#define I2S_DOUT    26

//==========================
// OPTIONAL
//==========================

#define AMP_EN      -1

// Motor Control Pin (Connect to NPN Transistor / MOSFET / Motor Driver IN pin)
#define MOTOR_PIN 14