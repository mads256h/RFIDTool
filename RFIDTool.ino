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
#include "templates.h"

#define RST_PIN 9 // Configurable, see typical pin layout above
#define SS_PIN 10 // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

// Indexes into the keys array to get either key A or key B.
byte keyAIndex[16] = {};
byte keyBIndex[16] = {};

bool hasRead = false;
byte block0Data[16] = {0x36, 0x93, 0x7F, 0x0C, 0xD6, 0x88, 0x04, 0x00, 0xC8, 0x40, 0x00, 0x20, 0x00, 0x00, 0x00, 0x18};
byte data[16 * 4 * 16] = {0};

void DumpSector(MFRC522::Uid *uid, MFRC522::MIFARE_Key *key, byte sector);

void DumpCard();
void ReadCard();
void VerifyCard();
void WriteCard();
void WriteUID();
void FindKeys();
void PrintKeys();
void GetTemplate();


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
                       "9. Get template\r\n"
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

        if (i >= '0' && i <= '9')
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

        for (int8_t i = 63; i >= 1; i--)
        {
            Serial.print(i);
            Serial.write(' ');
            if (i < 10)
                Serial.write(' ');

            PrintBlock(data + (i * 16));
        }

        PrintBlock0Formatted(data);

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
    case 9:
        GetTemplate();
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
        DumpSector(&mfrc522.uid, GetKey(keyAIndex[i], &key), i);
        //mfrc522.PICC_DumpMifareClassicSectorToSerial(&mfrc522.uid, GetKey(keyAIndex[i], &key), i);
    }

    HaltRFID(mfrc522);
}

//Reads a card using key A from Find keys.
void ReadCard()
{
    WaitForCard(mfrc522);

    for (int8_t i = 63; i >= 0; i--)
    {
        WaitForCardNoPrint(mfrc522);

        byte buffer[18] = {};
        byte len = sizeof(buffer);

        MFRC522::MIFARE_Key key = {};

        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, i, GetKey(keyAIndex[i / 4], &key), &mfrc522.uid);
        //PrintKey(key);
        Serial.print(F("Block "));
        Serial.print(i);
        Serial.print(F(" auth status: "));
        HandleStatusError(status, mfrc522);

        status = mfrc522.MIFARE_Read(i, buffer, &len);
        Serial.print(F("Block "));
        Serial.print(i);
        Serial.print(F(" read status: "));
        HandleStatusError(status, mfrc522);

        memcpy(data + (i * 16), buffer, 16);

        if (i % 4 == 3)
        {
            memcpy(data + (i * 16), key.keyByte, 6);

            GetKey(keyBIndex[i / 4], &key);
            memcpy(data + (i * 16) + 10, key.keyByte, 6);
        }

        HaltRFID(mfrc522);
    }

    hasRead = true;

    Terminal::Success(F("Read SUCCESS!"));

    HaltRFID(mfrc522);
    delay(1);
    return;

error:
    Terminal::Error(F("Read FAILED!"));
    HaltRFID(mfrc522);
}

void VerifyCard()
{
    CheckHasRead();

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
    CheckHasRead();

    if (!Terminal::Confirm(F("Proceed with writing to the card? This cannot be undone!")))
        return;

    WaitForCard(mfrc522);

    for (byte i = 0; i < 64; i++)
    {
        WaitForCardNoPrint(mfrc522);

        MFRC522::MIFARE_Key key = {};
        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, i, GetKey(keyBIndex[i / 4], &key), &mfrc522.uid);
        Serial.print(F("Block "));
        Serial.print(i);
        Serial.print(F(" auth status: "));
        HandleStatusError(status, mfrc522);

        status = mfrc522.MIFARE_Write(i, data + (i * 16), 16);
        Serial.print(F("Block "));
        Serial.print(i);
        Serial.print(F(" write status: "));
        HandleStatusError(status, mfrc522);

        HaltRFID(mfrc522);
        continue;

    error:
        Terminal::Error(F("Write FAILED!"));
        HaltRFID(mfrc522);
        continue;
    }

    Serial.println(F("Remember to verify the write"));
}

void WriteUID()
{
    CheckHasRead();

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

void DumpSector(MFRC522::Uid *uid,        ///< Pointer to Uid struct returned from a successful PICC_Select().
                MFRC522::MIFARE_Key *key, ///< Key A for the sector.
                byte sector               ///< The sector to dump, 0..39.
)
{
    MFRC522::StatusCode status;
    byte firstBlock;      // Address of lowest address to dump actually last block dumped)
    byte no_of_blocks;    // Number of blocks in sector
    bool isSectorTrailer; // Set to true while handling the "last" (ie highest address) in the sector.

    // The access bits are stored in a peculiar fashion.
    // There are four groups:
    //		g[3]	Access bits for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
    //		g[2]	Access bits for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
    //		g[1]	Access bits for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
    //		g[0]	Access bits for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)
    // Each group has access bits [C1 C2 C3]. In this code C1 is MSB and C3 is LSB.
    // The four CX bits are stored together in a nible cx and an inverted nible cx_.
    byte c1, c2, c3;    // Nibbles
    byte c1_, c2_, c3_; // Inverted nibbles
    bool invertedError; // True if one of the inverted nibbles did not match
    byte g[4];          // Access bits for each of the four groups.
    byte group;         // 0-3 - active group for access bits
    bool firstInGroup;  // True for the first block dumped in the group

    // Determine position and size of sector.
    if (sector < 32)
    { // Sectors 0..31 has 4 blocks each
        no_of_blocks = 4;
        firstBlock = sector * no_of_blocks;
    }
    else if (sector < 40)
    { // Sectors 32-39 has 16 blocks each
        no_of_blocks = 16;
        firstBlock = 128 + (sector - 32) * no_of_blocks;
    }
    else
    { // Illegal input, no MIFARE Classic PICC has more than 40 sectors.
        return;
    }

    // Dump blocks, highest address first.
    byte byteCount;
    byte buffer[18];
    byte blockAddr;
    isSectorTrailer = true;
    invertedError = false; // Avoid "unused variable" warning.
    for (int8_t blockOffset = no_of_blocks - 1; blockOffset >= 0; blockOffset--)
    {
        blockAddr = firstBlock + blockOffset;
        // Sector number - only on first line
        if (isSectorTrailer)
        {
            if (sector < 10)
                Serial.print(F("   ")); // Pad with spaces
            else
                Serial.print(F("  ")); // Pad with spaces
            Serial.print(sector);
            Serial.print(F("   "));
        }
        else
        {
            Serial.print(F("       "));
        }
        // Block number
        if (blockAddr < 10)
            Serial.print(F("   ")); // Pad with spaces
        else
        {
            if (blockAddr < 100)
                Serial.print(F("  ")); // Pad with spaces
            else
                Serial.print(F(" ")); // Pad with spaces
        }
        Serial.print(blockAddr);
        Serial.print(F("  "));
        // Establish encrypted communications before reading the first block
        if (isSectorTrailer)
        {
            status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, firstBlock, key, uid);
            if (status != MFRC522::STATUS_OK)
            {
                Serial.print(F("PCD_Authenticate() failed: "));
                Serial.println(MFRC522::GetStatusCodeName(status));
                return;
            }
        }
        // Read block
        byteCount = sizeof(buffer);
        status = mfrc522.MIFARE_Read(blockAddr, buffer, &byteCount);
        if (status != MFRC522::STATUS_OK)
        {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(MFRC522::GetStatusCodeName(status));
            continue;
        }

        if (isSectorTrailer)
        {
            MFRC522::MIFARE_Key key = {};
            GetKey(keyAIndex[sector], &key);
            memcpy(buffer, key.keyByte, sizeof(key.keyByte));

            GetKey(keyBIndex[sector], &key);
            memcpy(buffer + 10, key.keyByte, sizeof(key.keyByte));
        }

        // Dump data
        for (byte index = 0; index < 16; index++)
        {
            if (buffer[index] < 0x10)
                Serial.print(F(" 0"));
            else
                Serial.print(F(" "));
            Serial.print(buffer[index], HEX);
            if ((index % 4) == 3)
            {
                Serial.print(F(" "));
            }
        }
        // Parse sector trailer data
        if (isSectorTrailer)
        {
            c1 = buffer[7] >> 4;
            c2 = buffer[8] & 0xF;
            c3 = buffer[8] >> 4;
            c1_ = buffer[6] & 0xF;
            c2_ = buffer[6] >> 4;
            c3_ = buffer[7] & 0xF;
            invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
            g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
            g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
            g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
            g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
            isSectorTrailer = false;
        }

        // Which access group is this block in?
        if (no_of_blocks == 4)
        {
            group = blockOffset;
            firstInGroup = true;
        }
        else
        {
            group = blockOffset / 5;
            firstInGroup = (group == 3) || (group != (blockOffset + 1) / 5);
        }

        if (firstInGroup)
        {
            // Print access bits
            Serial.print(F(" [ "));
            Serial.print((g[group] >> 2) & 1, DEC);
            Serial.print(F(" "));
            Serial.print((g[group] >> 1) & 1, DEC);
            Serial.print(F(" "));
            Serial.print((g[group] >> 0) & 1, DEC);
            Serial.print(F(" ] "));
            if (invertedError)
            {
                Serial.print(F(" Inverted access bits did not match! "));
            }
        }

        if (group != 3 && (g[group] == 1 || g[group] == 6))
        { // Not a sector trailer, a value block
            int32_t value = (int32_t(buffer[3]) << 24) | (int32_t(buffer[2]) << 16) | (int32_t(buffer[1]) << 8) | int32_t(buffer[0]);
            Serial.print(F(" Value=0x"));
            Serial.print(value, HEX);
            Serial.print(F(" Adr=0x"));
            Serial.print(buffer[12], HEX);
        }
        Serial.println();
    }

    return;
} // End PICC_DumpMifareClassicSectorToSerial()

void GetTemplate()
{

    Serial.println(F("Select Template:\r\n\r\n1. Blank"));

    while (!Serial.available())
    {
    };

    const bool oldHasRead = hasRead;

    hasRead = true;

    switch (Serial.read() - '0')
    {
    case 1:
        ApplyTemplate(data, Template::Blank);
        break;

    default:
        Terminal::Error(F("Invalid selection."));
        hasRead = oldHasRead;
        break;
    }
}