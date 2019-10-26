#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>

namespace Terminal
{
static void Format(const byte formatting)
{
    Serial.print(F("\033["));
    Serial.print(formatting);
    Serial.write('m');
}

enum class Formatting : byte
{
    Bold = 1,
    Dim = 2,
    Underlined = 4,
    Blink = 5,
    Reverse = 7,
    Hidden = 8,
    ResetAll = 0
};

enum class Color : byte
{
    Default = 39,
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    LightGray = 37,
    DarkGray = 90,
    LightRed = 91,
    LightGreen = 92,
    LightYellow = 93,
    LightBlue = 94,
    LightMagenta = 95,
    LightCyan = 96,
    White = 97
};

static Color ColorToBackground(const Color color)
{
    return (Color)((byte)color + 10);
}

static Formatting FormattingToReset(const Formatting formatting)
{
    return (Formatting)((byte)formatting + 20);
}

static void Clear()
{
    Serial.print(F("\033[H\033[J"));
}

static void Bell()
{
    Serial.write(((byte)7));
}

static void ResetAll()
{
    Format((byte)Formatting::ResetAll);
}

template <typename T>
static void PrintWithFormatting(T printable, const byte formatting)
{
    Format(formatting);
    Serial.print(printable);
    ResetAll();
}

template <typename T>
static void PrintWithFormattingLn(T printable, const byte formatting)
{
    PrintWithFormatting<T>(printable, formatting);
    Serial.println();
}

template <typename T>
static void Success(T printable)
{
    PrintWithFormattingLn<T>(printable, (byte)Color::Green);
}

template <typename T>
static void Warn(T printable)
{
    PrintWithFormattingLn<T>(printable, (byte)Color::Yellow);
}

template <typename T>
static void Error(T printable)
{
    PrintWithFormattingLn<T>(printable, (byte)Color::Red);
}

} // namespace Terminal

#endif