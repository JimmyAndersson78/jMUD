/******************************************************************************
 * file: config.h
 *
 * created on: 2012-02-19 by jimmy
 * authors: jimmy
 *****************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include "target.h"         // What kind of machine/system are we compiling on?
#include "debug.h"          // Debug macros.


#include <cstdint>

#if (PLATFORM == PLATFORM_WINDOWS)
    #ifndef NULL
        #if defined(_Windows)
            #include <_null.h>
        #endif
    #endif
#else
    #ifndef NULL
        #define NULL        (0)
    #endif
#endif

#include <cstddef>          // std::size_t

typedef int8_t   int8;      //  8-bit signed integer
typedef uint8_t  uint8;     //  8-bit unsigned integer
typedef int16_t  int16;     // 16-bit signed integer
typedef uint16_t uint16;    // 16-bit unsigned integer
typedef int32_t  int32;     // 32-bit signed integer
typedef uint32_t uint32;    // 32-bit unsigned integer
typedef int64_t  int64;     // 64-bit signed integer
typedef uint64_t uint64;    // 64-bit unsigned integer


#include "globals.h"        // Global variables/objects/...


/* ------------------------------------------------------------------------- *
 * The minimum time that should pass between two pulses in the main loop.
 * Decreasing this value will most likely only make the program use more CPU
 * time without any increased game performance in any way. Increasing it may
 * decrease CPU usage without causing noticeable game performance slowdown.
 * ------------------------------------------------------------------------- */
const int64 DEF_CyclesPerSecond             = 10;
const int   DEF_CycleLengthMilliseconds     = 1000 / DEF_CyclesPerSecond;

const int DEF_TicksPerMinute =  60 * DEF_CyclesPerSecond;
const int DEF_TicksPerHour   =  60 * DEF_TicksPerMinute;
const int DEF_TicksPerDay    =  24 * DEF_TicksPerHour;
const int DEF_TicksPerYear   = 365 * DEF_TicksPerDay;


const std::size_t TIME_IdleVoid                  =  8 * 60;  // Idle to Voided time (s)
const std::size_t TIME_IdleDisconnect            = 24 * 60;  // Idle to Disconnected time (s)
const std::size_t TIME_ConnectTimeout            =  2 * 60;  // Connected to Disconnected (s)

const std::size_t SIZE_MaxBufferSize             = 32768;

const std::size_t SIZE_SmallBuffer              = 1024 - 16;    // Memory allocations can contain "hidden"
const std::size_t SIZE_LargeBuffer              = 4096 - 16;    // (from us) headers, so try to make sure
const std::size_t SIZE_MaxBuffer                = 4096*8 - 16;  // we don't cause allocations that are just
                                                                // just above X * pagesize large.
const std::size_t SIZE_MaxScriptLength          = SIZE_MaxBuffer;

const std::size_t LIMIT_MaxAccountNameLength     = 31;
const std::size_t LIMIT_MinAccountNameLength     = 4;
const std::size_t LIMIT_MaxPasswordLength        = 63;
const std::size_t LIMIT_MinPasswordLength        = 1; // = 6
const std::size_t LIMIT_MaxNameLength            = 31;
const std::size_t LIMIT_MaxTitleLength           = 31;


/* ------------------------------------------------------------------------- *
 * Some temporary constant strings.
 * ------------------------------------------------------------------------- */
const char MSG_BufferOverFlow[] = "\r\n*** BUFFER OVERFLOW ***\r\n";

const char MSG_GameName[]       = "jMUD-test";
const char MSG_Welcome[]        = "\r\nWelcome to jMUD. We hope you will enjoy your stay.\r\n\r\n";
const char MSG_GameFull[]       = "Game is full. Please try again later!\r\n";
const char MSG_GameUnstable[]   = "Game is unstable! Try again later.\r\n";

const char MSG_LoginPrompt[]    = "Enter your account name: ";
const char MSG_NameQuestion[]   = "\r\n\r\nBy what name do you wish to be known? ";
const char MSG_GoodBye[]        = "Bye! We hope to see you soon again!\r\n";
const char MSG_AccountMenu[]    = "\r\n\r\njMUD AccountMenu:\r\n\r\n  list - Lists all characters in the account.\r\n  play <name> - Starts playing the character named <name>, if it exists.\r\n  create <name> - Creates a character with named <name>, if <name> is available.\r\n  exit - Quits the account menu, and thus the game.\r\n\r\nAccount> ";
const char MSG_GamePrompt[]     = "\r\n> ";

const char MSG_HostBanned[]     = "\r\n\r\nYour host is banned!\r\n\r\nContact an administrator of the game if you feel like you have been banned without reasonable cause. Though remember that it's the administrators that have to find the reason reasonable, not you.";
const char MSG_ContactAdmins[]  = "\r\nPlease contact the appropriate administrator to solve your problem:\r\nMachine: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>\r\nGame: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>\r\n\r\n";
const char MSG_GameShutdown[]   = "\r\nManual game shutdown. Please come back later.\r\n";
const char MSG_AutoReboot[]     = "\r\nAutomatic reboot. The game will be up again in a short while.\r\n";

// These should be remade so that they are read from files at startup.
const char MSG_WelcomeScreen[]  = "\r\nWelcome to jMUD (This is a VERY early alpha and lacks a LOT.)\r\n\r\n\r\nIf you don't have an account and want to make one,\r\ntype \"NEW\" as account name to create one.\r\n\r\n";
const char MSG_AccountLicense[] = "\r\n1) You have no rights at all from/in the game.\r\n\r\nDo you accept the terms presented above? (yes/no) ";

#endif /* CONFIG_H */
