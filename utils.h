#ifndef UTILS_H
#define UTILS_H

#include <MFRC522.h>
#include "terminal.h"

// Resets the atmega CPU.
void (*Reset)() = 0;

static void PrintBlock0Formatted(byte *buffer)
{
    //            "UID + BCC   SAK+ATAQ Manufacturer data"
    //char *buf = "00:00:00:00:00 00:00 00:00:00:00:00:00:00:00:00";

    Terminal::PrintWithFormatting(F("UID + BCC   "), (byte)Terminal::Color::Blue);
    Terminal::PrintWithFormatting(F("SAK+ATAQ "), (byte)Terminal::Color::Green);
    Terminal::PrintWithFormattingLn(F("Manufacturer data"), (byte)Terminal::Color::Yellow);

    char buf[30] = {};

    snprintf_P(buf, sizeof(buf), PSTR("%.2X:%.2X:%.2X:%.2X:%.2X "), buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    Terminal::PrintWithFormatting(buf, (byte)Terminal::Color::Blue);

    snprintf_P(buf, sizeof(buf), PSTR("%.2X:%.2X "), buffer[5], buffer[6]);
    Terminal::PrintWithFormatting(buf, (byte)Terminal::Color::Green);

    snprintf_P(buf, sizeof(buf), PSTR("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X:%.2X"), buffer[7], buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);
    Terminal::PrintWithFormattingLn(buf, (byte)Terminal::Color::Yellow);
}

static void PrintBlock(byte *buffer){
    char buf[64] = {};
    snprintf_P(buf, sizeof(buf), PSTR("%.2X:%.2X:%.2X:%.2X %.2X:%.2X:%.2X:%.2X %.2X:%.2X:%.2X:%.2X %.2X:%.2X:%.2X:%.2X"), buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);
    Serial.println(buf);
}

static void PrintKey(const MFRC522::MIFARE_Key &key)
{
    char buf[15] = {};
    snprintf_P(buf, sizeof(buf), PSTR("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X"), key.keyByte[0], key.keyByte[1], key.keyByte[2], key.keyByte[3], key.keyByte[4], key.keyByte[5]);
    Terminal::PrintWithFormattingLn(buf, (byte)Terminal::Color::Yellow);
}

static void WaitForCardNoPrint(MFRC522 &mfrc522)
{
    byte atqa_answer[2] = {0};
    byte atqa_size = 2;
    while (!(mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_WakeupA(atqa_answer, &atqa_size) == MFRC522::StatusCode::STATUS_OK) || !mfrc522.PICC_ReadCardSerial())
    {
        yield();
    }
}

static void WaitForCard(MFRC522 &mfrc522)
{
    Terminal::PrintWithFormattingLn(F("Place card in front of RFID reader"), (byte)Terminal::Color::Blue);

    WaitForCardNoPrint(mfrc522);

    Serial.println(F("Card found!"));
}

static void HaltRFID(MFRC522 &mfrc522)
{
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}


#define HandleStatusError(status, mfrc522)                       \
    {                                                            \
        if (status != MFRC522::StatusCode::STATUS_OK)            \
        {                                                        \
            Terminal::Error(MFRC522::GetStatusCodeName(status)); \
            HaltRFID(mfrc522);                                   \
            goto error;                                          \
        }                                                        \
        Terminal::Success(MFRC522::GetStatusCodeName(status));   \
    }

#endif