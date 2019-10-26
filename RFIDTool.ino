/*
 * Do not use the standard Arduino Serial monitor:
 * It does not work well with this program.
 * Instead use putty on windows and screen on macos/linux.
 * -----------------------------------------------------------------------------------------
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */

#include <SPI.h>
#include <MFRC522.h>
#include "utils.h"
#include "terminal.h"
#include "keys.h"

#define RST_PIN 9 // Configurable, see typical pin layout above
#define SS_PIN 10 // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

// Indexes into the keys array to get either key A or key B.
byte keyAIndex[16] = {};
byte keyBIndex[16] = {};

bool hasRead = false;
byte block0Data[16] = {0x36, 0x93, 0x7F, 0x0C, 0xD6, 0x88, 0x04, 0x00, 0xC8, 0x40, 0x00, 0x20, 0x00, 0x00, 0x00, 0x18};

void setup()
{
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    delay(4);
    Terminal::Clear();
}

//Prints options and gets a valid selection.
int GetUserSelection()
{
    while (true)
    {
        Serial.print(F("1. Dump card\r\n"
                       "2. Read card\r\n"
                       "3. Print data\r\n"
                       "4. Verify data\r\n"
                       "5. Write card\r\n"
                       "6. Write UID\r\n"
                       "7. Find keys\r\n"
                       "8. Print keys\r\n"
                       "0. Reset\r\n"
                       "[RFIDTool@Arduino]$ "));
        while (!Serial.available())
        {
            yield();
        }

        const int i = Serial.read();
        if (i == -1)
            continue;
        Serial.write(i);
        Serial.println();

        if (i >= '0' && i <= '8')
            return i - '0';

        Terminal::Error(F("Invalid input"));
    }
}

void loop()
{
    switch (GetUserSelection())
    {
    case 1:
        DumpCard();
        break;
    case 2:
        ReadCard();
        break;
    case 3:
        if (!hasRead)
        {
            Terminal::Error(F("Read a card first!"));
            break;
        }
        Serial.println(F("Stored block 0:"));
        PrintBlock0Formatted(block0Data);
        break;
    case 4:
        VerifyCard();
        break;
    case 5:
        WriteCard();
        break;
    case 6:
        WriteUID();
        break;
    case 7:
        FindKeys();
        break;
    case 8:
        PrintKeys();
        break;
    case 0:
        Reset();
        break;
    }
    Serial.println();
}

void DumpCard()
{
    WaitForCard(mfrc522);

    MFRC522::MIFARE_Key key = {};

    for (int8_t i = 15; i >= 0; i--)
    {
        mfrc522.PICC_DumpMifareClassicSectorToSerial(&mfrc522.uid, GetKey(keyAIndex[i], &key), i);
    }

    HaltRFID(mfrc522);
} 


//Reads a card using key A from Find keys.
void ReadCard()
{
    WaitForCard(mfrc522);

    byte buffer[18] = {};
    byte len = sizeof(buffer);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 0, GetKey(keyAIndex[0], &key), &mfrc522.uid);
    Serial.print(F("Block 0 auth status: "));
    HandleStatusError(status, mfrc522);

    status = mfrc522.MIFARE_Read(0, buffer, &len);
    Serial.print(F("Block 0 read status: "));
    HandleStatusError(status, mfrc522);

    Serial.println(F("Read following 16 bytes:"));
    PrintBlock0Formatted(buffer);

    memcpy(block0Data, buffer, sizeof(block0Data));
    hasRead = true;

    Terminal::Success(F("Read SUCCESS!"));

    HaltRFID(mfrc522);
    return;

error:
    Terminal::Error(F("Read FAILED!"));
}

void VerifyCard()
{
    if (!hasRead)
    {
        Terminal::Error(F("Read a card first!"));
        return;
    }

    byte buffer[18] = {};
    byte len = sizeof(buffer);

    WaitForCard(mfrc522);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 0, GetKey(keyAIndex[0], &key), &mfrc522.uid);
    Serial.print(F("Block 0 auth status: "));
    HandleStatusError(status, mfrc522);

    status = mfrc522.MIFARE_Read(0, buffer, &len);
    Serial.print(F("Block 0 read status: "));
    HandleStatusError(status, mfrc522);

    HaltRFID(mfrc522);
    if (memcmp(buffer, block0Data, sizeof(block0Data)) == 0)
    {
        Terminal::Success(F("Verify SUCCESS!"));
        return;
    }

error:
    Terminal::Error(F("Verify FAILED!"));
}

void WriteCard()
{
    if (!hasRead)
    {
        Terminal::Error(F("Read a card first!"));
        return;
    }

    WaitForCard(mfrc522);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 0, GetKey(keyBIndex[0], &key), &mfrc522.uid);
    Serial.print(F("Block 0 auth status: "));
    HandleStatusError(status, mfrc522);

    status = mfrc522.MIFARE_Write(0, block0Data, 16);
    Serial.print(F("Block 0 write status: "));
    HandleStatusError(status, mfrc522);

    Terminal::Success(F("Write SUCCESS!"));
    Serial.println(F("Remember to verify the write"));

    HaltRFID(mfrc522);
    return;

error:
    Terminal::Error(F("Write FAILED!"));
}

void WriteUID()
{
    if (!hasRead)
    {
        Terminal::Error(F("Read a card first!"));
        return;
    }

    WaitForCard(mfrc522);

    const bool success = mfrc522.MIFARE_SetUid(block0Data, 5, true);

    if (success)
    {
        Terminal::Success(F("UID write succeeded"));
    }
    else
    {
        Terminal::Error(F("UID write failed"));
    }

    HaltRFID(mfrc522);
}

void FindKeys()
{
    WaitForCard(mfrc522);
    Terminal::Warn(F("This might take some time."));

    bool found = false;

    MFRC522::MIFARE_Key key = {};

    for (byte j = 0; j < 16; j++)
    {
        for (size_t i = 0; i < defaultKeysLength; i++)
        {
            WaitForCardNoPrint(mfrc522);

            const MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, j * 4, GetKey(i, &key), &mfrc522.uid);
            HaltRFID(mfrc522);
            if (status != MFRC522::StatusCode::STATUS_OK)
            {
                continue;
            }

            keyAIndex[j] = i;
            found = true;
            break;
        }

        if (!found)
        {
            Terminal::Error(F("Did not find key A in sector "));
            Terminal::Error(j);
        }

        found = false;

        for (size_t i = 0; i < defaultKeysLength; i++)
        {
            WaitForCardNoPrint(mfrc522);

            const MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, j * 4, GetKey(i, &key), &mfrc522.uid);
            HaltRFID(mfrc522);
            if (status != MFRC522::StatusCode::STATUS_OK)
            {
                continue;
            }

            keyBIndex[j] = i;
            found = true;
            break;
        }

        if (!found)
        {
            Terminal::Error(F("Did not find key B in sector "));
            Terminal::Error(j);
        }
    }
}

void PrintKeys()
{
    MFRC522::MIFARE_Key key = {};

    for (byte i = 0; i < sizeof(keyAIndex); i++)
    {
        GetKey(keyAIndex[i], &key);
        Serial.print(F("Key A sector "));
        Serial.print(i);
        Serial.print(F(": "));
        PrintKey(key);

        GetKey(keyBIndex[i], &key);
        Serial.print(F("Key B sector "));
        Serial.print(i);
        Serial.print(F(": "));
        PrintKey(key);
    }
}