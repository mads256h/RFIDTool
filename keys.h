#ifndef KEYS_H
#define KEYS_H

#include <avr/pgmspace.h>
#include <MFRC522.h>


// clang-format off
//TODO: add more keys
//Do not use this array directly.
const byte defaultKeys[] PROGMEM = {
    //ffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    //a0b0c0d0e0f0
    0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
    //a1b1c1d1e1f1
    0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1,
    //a0a1a2a3a4a5
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
    //b0b1b2b3b4b5
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5,
    //4d3a99c351dd
    0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD,
    //1a982c7e459a
    0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A,
    //000000000000
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //d3f7d3f7d3f7
    0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7,
    //aabbccddeeff
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
};
// clang-format on

constexpr size_t defaultKeysLength = sizeof(defaultKeys) / sizeof(MFRC522::MIFARE_Key);

//Gets a key from flash memory.
MFRC522::MIFARE_Key * GetKey(const size_t i, MFRC522::MIFARE_Key * const buffer)
{
    for (size_t j = 0; j < sizeof(MFRC522::MIFARE_Key); j++)
    {
        buffer->keyByte[j] = pgm_read_byte(defaultKeys + (i * sizeof(MFRC522::MIFARE_Key)) + j);
    }

    return buffer;
}

#endif