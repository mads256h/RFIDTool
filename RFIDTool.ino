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
byte keyAIndex = 0; 
byte keyBIndex = 0;

bool hasRead = true;
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
        Serial.print(F("1. Read card\r\n"
                       "2. Print data\r\n"
                       "3. Verify data\r\n"
                       "4. Write card\r\n"
                       "5. Write UID\r\n"
                       "6. Find keys\r\n"
                       "7. Print keys\r\n"
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

        if (i >= '0' && i <= '7')
            return i - '0';

        Terminal::Error(F("Invalid input"));
    }
}

void loop()
{
    switch (GetUserSelection())
    {
    case 1:
        ReadCard();
        break;
    case 2:
        Serial.println(F("Stored block 0:"));
        PrintBlock0Formatted(block0Data);
        break;
    case 3:
        VerifyCard();
        break;
    case 4:
        WriteCard();
        break;
    case 5:
        WriteUID();
        break;
    case 6:
        FindKeys();
        break;
    case 7:
        PrintKeys();
        break;
    case 0:
        Reset();
        break;
    }
    Serial.println();
}

//Reads a card using key A from Find keys.
void ReadCard()
{
    WaitForCard(mfrc522);

    mfrc522.PICC_DumpDetailsToSerial(&mfrc522.uid);

    byte buffer[18] = {};
    byte len = sizeof(buffer);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 0, GetKey(keyAIndex, &key), &mfrc522.uid);
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
        Serial.println(F("Read a card first!"));
        return;
    }

    byte buffer[18] = {};
    byte len = sizeof(buffer);

    WaitForCard(mfrc522);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 0, GetKey(keyAIndex, &key), &mfrc522.uid);
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
        Serial.println(F("Read a card first!"));
        return;
    }

    WaitForCard(mfrc522);

    MFRC522::MIFARE_Key key = {};
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 0, GetKey(keyBIndex, &key), &mfrc522.uid);
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
        Serial.println(F("Read a card first!"));
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
    Terminal::PrintWithFormattingLn(F("Place card in front of RFID reader"), (byte)Terminal::Color::Blue);
    Serial.println(F("Trying to find key A..."));

    bool found = false;

    MFRC522::MIFARE_Key key = {};

    for (size_t i = 0; i < defaultKeysLength; i++)
    {
        WaitForCardNoPrint(mfrc522);

        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 0, GetKey(i, &key), &mfrc522.uid);
        if (status == MFRC522::StatusCode::STATUS_OK)
        {
            keyAIndex = i;
            Terminal::Success(F("Found key A!"));
            PrintKey(key);
            HaltRFID(mfrc522);
            found = true;
            break;
        }

        HaltRFID(mfrc522);
    }

    if (!found)
    {
        Terminal::Error(F("Did not find key A!"));
    }

    found = false;

    Serial.println(F("Trying to find key B..."));
    for (size_t i = 0; i < defaultKeysLength; i++)
    {
        WaitForCardNoPrint(mfrc522);

        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 0, GetKey(i, &key), &mfrc522.uid);
        if (status == MFRC522::StatusCode::STATUS_OK)
        {
            keyBIndex = i;
            Terminal::Success(F("Found key B!"));
            PrintKey(key);
            HaltRFID(mfrc522);
            found = true;
            break;
        }

        HaltRFID(mfrc522);
    }

    if (!found)
    {
        Terminal::Error(F("Did not find key B!"));
    }
}

void PrintKeys()
{
    MFRC522::MIFARE_Key key = {};
    GetKey(keyAIndex, &key);
    Serial.print(F("Key A: "));
    PrintKey(key);

    GetKey(keyBIndex, &key);
    Serial.print(F("Key B: "));
    PrintKey(key);
}