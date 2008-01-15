/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NINTENDO_ES_USAGE_H_INCLUDED
#define NINTENDO_ES_USAGE_H_INCLUDED

#include <es/types.h>

namespace UsageID
{
    static const u8 KEYBOARD_ERRORROLLOVER = 0x01;
    static const u8 KEYBOARD_POSTFAIL = 0x02;
    static const u8 KEYBOARD_ERRORUNDEFINED = 0x03;
    static const u8 KEYBOARD_A = 0x04;
    static const u8 KEYBOARD_B = 0x05;
    static const u8 KEYBOARD_C = 0x06;
    static const u8 KEYBOARD_D = 0x07;
    static const u8 KEYBOARD_E = 0x08;
    static const u8 KEYBOARD_F = 0x09;
    static const u8 KEYBOARD_G = 0x0A;
    static const u8 KEYBOARD_H = 0x0B;
    static const u8 KEYBOARD_I = 0x0C;
    static const u8 KEYBOARD_J = 0x0D;
    static const u8 KEYBOARD_K = 0x0E;
    static const u8 KEYBOARD_L = 0x0F;
    static const u8 KEYBOARD_M = 0x10;
    static const u8 KEYBOARD_N = 0x11;
    static const u8 KEYBOARD_O = 0x12;
    static const u8 KEYBOARD_P = 0x13;
    static const u8 KEYBOARD_Q = 0x14;
    static const u8 KEYBOARD_R = 0x15;
    static const u8 KEYBOARD_S = 0x16;
    static const u8 KEYBOARD_T = 0x17;
    static const u8 KEYBOARD_U = 0x18;
    static const u8 KEYBOARD_V = 0x19;
    static const u8 KEYBOARD_W = 0x1A;
    static const u8 KEYBOARD_X = 0x1B;
    static const u8 KEYBOARD_Y = 0x1C;
    static const u8 KEYBOARD_Z = 0x1D;
    static const u8 KEYBOARD_1 = 0x1E;
    static const u8 KEYBOARD_2 = 0x1F;
    static const u8 KEYBOARD_3 = 0x20;
    static const u8 KEYBOARD_4 = 0x21;
    static const u8 KEYBOARD_5 = 0x22;
    static const u8 KEYBOARD_6 = 0x23;
    static const u8 KEYBOARD_7 = 0x24;
    static const u8 KEYBOARD_8 = 0x25;
    static const u8 KEYBOARD_9 = 0x26;
    static const u8 KEYBOARD_0 = 0x27;
    static const u8 KEYBOARD_ENTER = 0x28;
    static const u8 KEYBOARD_ESCAPE = 0x29;
    static const u8 KEYBOARD_BACKSPACE = 0x2A;
    static const u8 KEYBOARD_TAB = 0x2B;
    static const u8 KEYBOARD_SPACEBAR = 0x2C;
    static const u8 KEYBOARD_MINUS = 0x2D;
    static const u8 KEYBOARD_EQUAL = 0x2E;
    static const u8 KEYBOARD_LEFT_BRACKET = 0x2F;
    static const u8 KEYBOARD_RIGHT_BRACKET = 0x30;
    static const u8 KEYBOARD_BACKSLASH = 0x31;
    static const u8 KEYBOARD_NON_US_HASH = 0x32;
    static const u8 KEYBOARD_SEMICOLON = 0x33;
    static const u8 KEYBOARD_QUOTE = 0x34;
    static const u8 KEYBOARD_GRAVE_ACCENT = 0x35;   // `
    static const u8 KEYBOARD_COMMA = 0x36;
    static const u8 KEYBOARD_PERIOD = 0x37;
    static const u8 KEYBOARD_SLASH = 0x38;
    static const u8 KEYBOARD_CAPS_LOCK = 0x39;
    static const u8 KEYBOARD_F1 = 0x3A;
    static const u8 KEYBOARD_F2 = 0x3B;
    static const u8 KEYBOARD_F3 = 0x3C;
    static const u8 KEYBOARD_F4 = 0x3D;
    static const u8 KEYBOARD_F5 = 0x3E;
    static const u8 KEYBOARD_F6 = 0x3F;
    static const u8 KEYBOARD_F7 = 0x40;
    static const u8 KEYBOARD_F8 = 0x41;
    static const u8 KEYBOARD_F9 = 0x42;
    static const u8 KEYBOARD_F10 = 0x43;
    static const u8 KEYBOARD_F11 = 0x44;
    static const u8 KEYBOARD_F12 = 0x45;
    static const u8 KEYBOARD_PRINTSCREEN = 0x46;
    static const u8 KEYBOARD_SCROLL_LOCK = 0x47;
    static const u8 KEYBOARD_PAUSE = 0x48;
    static const u8 KEYBOARD_INSERT = 0x49;
    static const u8 KEYBOARD_HOME = 0x4A;
    static const u8 KEYBOARD_PAGEUP = 0x4B;
    static const u8 KEYBOARD_DELETE = 0x4C;
    static const u8 KEYBOARD_END = 0x4D;
    static const u8 KEYBOARD_PAGEDOWN = 0x4E;
    static const u8 KEYBOARD_RIGHTARROW = 0x4F;
    static const u8 KEYBOARD_LEFTARROW = 0x50;
    static const u8 KEYBOARD_DOWNARROW = 0x51;
    static const u8 KEYBOARD_UPARROW = 0x52;
    static const u8 KEYPAD_NUM_LOCK = 0x53;
    static const u8 KEYPAD_DIVIDE = 0x54;
    static const u8 KEYPAD_MULTIPLY = 0x55;
    static const u8 KEYPAD_SUBTRACT = 0x56;
    static const u8 KEYPAD_ADD = 0x57;
    static const u8 KEYPAD_ENTER = 0x58;
    static const u8 KEYPAD_1 = 0x59;
    static const u8 KEYPAD_2 = 0x5A;
    static const u8 KEYPAD_3 = 0x5B;
    static const u8 KEYPAD_4 = 0x5C;
    static const u8 KEYPAD_5 = 0x5D;
    static const u8 KEYPAD_6 = 0x5E;
    static const u8 KEYPAD_7 = 0x5F;
    static const u8 KEYPAD_8 = 0x60;
    static const u8 KEYPAD_9 = 0x61;
    static const u8 KEYPAD_0 = 0x62;
    static const u8 KEYPAD_DOT = 0x63;
    static const u8 KEYBOARD_NON_US_BACKSLASH = 0x64;
    static const u8 KEYBOARD_APPLICATION = 0x65;
    static const u8 KEYBOARD_POWER = 0x66;
    static const u8 KEYPAD_EQUAL = 0x67;
    static const u8 KEYBOARD_F13 = 0x68;
    static const u8 KEYBOARD_F14 = 0x69;
    static const u8 KEYBOARD_F15 = 0x6A;
    static const u8 KEYBOARD_F16 = 0x6B;
    static const u8 KEYBOARD_F17 = 0x6C;
    static const u8 KEYBOARD_F18 = 0x6D;
    static const u8 KEYBOARD_F19 = 0x6E;
    static const u8 KEYBOARD_F20 = 0x6F;
    static const u8 KEYBOARD_F21 = 0x70;
    static const u8 KEYBOARD_F22 = 0x71;
    static const u8 KEYBOARD_F23 = 0x72;
    static const u8 KEYBOARD_F24 = 0x73;
    static const u8 KEYBOARD_EXECUTE = 0x74;
    static const u8 KEYBOARD_HELP = 0x75;
    static const u8 KEYBOARD_MENU = 0x76;
    static const u8 KEYBOARD_SELECT = 0x77;
    static const u8 KEYBOARD_STOP = 0x78;
    static const u8 KEYBOARD_AGAIN = 0x79;
    static const u8 KEYBOARD_UNDO = 0x7A;
    static const u8 KEYBOARD_CUT = 0x7B;
    static const u8 KEYBOARD_COPY = 0x7C;
    static const u8 KEYBOARD_PASTE = 0x7D;
    static const u8 KEYBOARD_FIND = 0x7E;
    static const u8 KEYBOARD_MUTE = 0x7F;
    static const u8 KEYBOARD_VOLUME_UP = 0x80;
    static const u8 KEYBOARD_VOLUME_DOWN = 0x81;
    static const u8 KEYBOARD_LOCKING_CAPS_LOCK = 0x82;
    static const u8 KEYBOARD_LOCKING_NUM_LOCK = 0x83;
    static const u8 KEYBOARD_LOCKING_SCROLL_LOCK = 0x84;
    static const u8 KEYPAD_COMMA = 0x85;
    static const u8 KEYPAD_EQUAL_SIGN = 0x86;
    static const u8 KEYBOARD_INTERNATIONAL1 = 0x87;
    static const u8 KEYBOARD_INTERNATIONAL2 = 0x88;
    static const u8 KEYBOARD_INTERNATIONAL3 = 0x89;
    static const u8 KEYBOARD_INTERNATIONAL4 = 0x8A;
    static const u8 KEYBOARD_INTERNATIONAL5 = 0x8B;
    static const u8 KEYBOARD_INTERNATIONAL6 = 0x8C;
    static const u8 KEYBOARD_INTERNATIONAL7 = 0x8D;
    static const u8 KEYBOARD_INTERNATIONAL8 = 0x8E;
    static const u8 KEYBOARD_INTERNATIONAL9 = 0x8F;
    static const u8 KEYBOARD_LANG1 = 0x90;
    static const u8 KEYBOARD_LANG2 = 0x91;
    static const u8 KEYBOARD_LANG3 = 0x92;
    static const u8 KEYBOARD_LANG4 = 0x93;
    static const u8 KEYBOARD_LANG5 = 0x94;
    static const u8 KEYBOARD_LANG6 = 0x95;
    static const u8 KEYBOARD_LANG7 = 0x96;
    static const u8 KEYBOARD_LANG8 = 0x97;
    static const u8 KEYBOARD_LANG9 = 0x98;
    static const u8 KEYBOARD_ALTERNATE_ERASE = 0x99;
    static const u8 KEYBOARD_SYSREQ_ATTENTION = 0x9A;
    static const u8 KEYBOARD_CANCEL = 0x9B;
    static const u8 KEYBOARD_CLEAR = 0x9C;
    static const u8 KEYBOARD_PRIOR = 0x9D;
    static const u8 KEYBOARD_RETURN = 0x9E;
    static const u8 KEYBOARD_SEPARATOR = 0x9F;
    static const u8 KEYBOARD_OUT = 0xA0;
    static const u8 KEYBOARD_OPER = 0xA1;
    static const u8 KEYBOARD_CLEAR_AGAIN = 0xA2;
    static const u8 KEYBOARD_CRSEL_PROPS = 0xA3;
    static const u8 KEYBOARD_EXSEL = 0xA4;
    static const u8 KEYPAD_00 = 0xB0;
    static const u8 KEYPAD_000 = 0xB1;
    static const u8 THOUSANDS_SEPARATOR = 0xB2;
    static const u8 DECIMAL_SEPARATOR = 0xB3;
    static const u8 CURRENCY_UNIT = 0xB4;
    static const u8 CURRENCY_SUB_UNIT = 0xB5;
    static const u8 KEYPAD_LEFT_PAREN = 0xB6;
    static const u8 KEYPAD_RIGHT_PAREN = 0xB7;
    static const u8 KEYPAD_LEFT_BRACE = 0xB8;
    static const u8 KEYPAD_RIGHT_BRACE = 0xB9;
    static const u8 KEYPAD_TAB = 0xBA;
    static const u8 KEYPAD_BACKSPACE = 0xBB;
    static const u8 KEYPAD_A = 0xBC;
    static const u8 KEYPAD_B = 0xBD;
    static const u8 KEYPAD_C = 0xBE;
    static const u8 KEYPAD_D = 0xBF;
    static const u8 KEYPAD_E = 0xC0;
    static const u8 KEYPAD_F = 0xC1;
    static const u8 KEYPAD_XOR = 0xC2;
    static const u8 KEYPAD_HAT = 0xC3;      // ^
    static const u8 KEYPAD_PERCENT = 0xC4;
    static const u8 KEYPAD_LESS = 0xC5;
    static const u8 KEYPAD_MORE = 0xC6;
    static const u8 KEYPAD_AND = 0xC7;
    static const u8 KEYPAD_LOGICAL_AND = 0xC8;
    static const u8 KEYPAD_OR = 0xC9;
    static const u8 KEYPAD_LOGICAL_OR = 0xCA;
    static const u8 KEYPAD_COLON = 0xCB;
    static const u8 KEYPAD_HASH = 0xCC;     // #
    static const u8 KEYPAD_SPACE = 0xCD;
    static const u8 KEYPAD_ATMARK = 0xCE;
    static const u8 KEYPAD_NOT = 0xCF;      // !
    static const u8 KEYPAD_MEMORY_STORE = 0xD0;
    static const u8 KEYPAD_MEMORY_RECALL = 0xD1;
    static const u8 KEYPAD_MEMORY_CLEAR = 0xD2;
    static const u8 KEYPAD_MEMORY_ADD = 0xD3;
    static const u8 KEYPAD_MEMORY_SUBTRACT = 0xD4;
    static const u8 KEYPAD_MEMORY_MULTIPLY = 0xD5;
    static const u8 KEYPAD_MEMORY_DIVIDE = 0xD6;
    static const u8 KEYPAD_SIGN = 0xD7;     // +/-
    static const u8 KEYPAD_CLEAR = 0xD8;
    static const u8 KEYPAD_CLEAR_ENTRY = 0xD9;
    static const u8 KEYPAD_BINARY = 0xDA;
    static const u8 KEYPAD_OCTAL = 0xDB;
    static const u8 KEYPAD_DECIMAL = 0xDC;
    static const u8 KEYPAD_HEXADECIMAL = 0xDD;
    static const u8 KEYBOARD_LEFTCONTROL = 0xE0;
    static const u8 KEYBOARD_LEFTSHIFT = 0xE1;
    static const u8 KEYBOARD_LEFTALT = 0xE2;
    static const u8 KEYBOARD_LEFT_GUI = 0xE3;
    static const u8 KEYBOARD_RIGHTCONTROL = 0xE4;
    static const u8 KEYBOARD_RIGHTSHIFT = 0xE5;
    static const u8 KEYBOARD_RIGHTALT = 0xE6;
    static const u8 KEYBOARD_RIGHT_GUI = 0xE7;
};

#endif  // #ifndef NINTENDO_ES_USAGE_H_INCLUDED
