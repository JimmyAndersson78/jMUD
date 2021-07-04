/******************************************************************************
 * file: target.h
 *
 * description: Tries to determine what kind of platform we are trying to
 *              compile on, and if a known platform is identified we should
 *              be able to compile without any errors.
 *
 * created: 2010-04-19 by jimmy
 * authors: jimmy
 *****************************************************************************/
#ifndef TARGET_H
#define TARGET_H


/* ------------------------------------------------------------------------- *
 * This file will define the symbols PLATFORM and SYSTEM, and also assign them
 * values to differentiate between the different versions for each symbol.
 *   If no known platform can be detected the value of PLATFORM will default
 * to the value of PLATFORM_DEFAULT. (It's the platform that is deemed to have
 * the most universally supported code and the version to require the least
 * alterations.)
 *
 * WARNING: The value of the symbol SYSTEM is ONLY valid in context with the
 *          PLATFORM_XXX symbol.
 * ------------------------------------------------------------------------- */
#define PLATFORM_UNIX               1   // UNIX like platforms
#define PLATFORM_WINDOWS            2   // Microsoft Windows
#define PLATFORM_BSD                3   // BSD like platforms
#define PLATFORM_UNKNOWN            0   // unknown platform.

#ifndef PLATFORM

    // Is this some type of *NIX system?
    #if defined(unix) || defined(_unix) || defined(__unix) || defined(__unix__)

        #define PLATFORM            PLATFORM_UNIX
        #define PLATFORM_STR        "UNIX"

        #define SYSTEM_UNIX         0   // Untested (should work, mostly)
        #define SYSTEM_LINUX        1   // Working
        #define SYSTEM_SUNOS        2   // Working
        #define SYSTEM_SOLARIS      3   // Untested (should work)
        #define SYSTEM_CYGWIN       4   // Working
        #define SYSTEM_DEFAULT      SYSTEM_UNIX

        #if defined(linux) || defined(_linux) || defined(__linux) || defined (__linux__)
            #define SYSTEM          SYSTEM_LINUX
            #define SYSTEM_STR      "Linux"
        #elif defined(sun) || defined(_sun) || defined(__sun) || defined(__sun__)
            #if defined(srv4) || defined(_srv4) || defined(__srv4) || defined(__srv4__) || defined(SRV4) || defined(_SRV4) || defined(__SRV4) || defined(__SRV4__)
                #define SYSTEM      SYSTEM_SOLARIS
                #define SYSTEM_STR  "Solaris"
            #else
                #define SYSTEM      SYSTEM_SUNOS
                #define SYSTEM_STR  "SunOS"
            #endif
        #elif defined(__CYGWIN__) || defined(__CYGWIN32__)
            #define SYSTEM          SYSTEM_CYGWIN
            #define SYSTEM_STR      "Cygwin"
        #else
            #define SYSTEM          SYSTEM_DEFAULT
            #define SYSTEM_STR      "Generic UNIX"
        #endif // defined(linux) || defined(_linux) || defined(__linux) || defined (__linux__)

    // Is this some type of Windows system?
    #elif defined(_WIN32) || defined(_Windows) || defined(WIN32) || defined (__WIN32)
        #warning "Microsoft Windows: support outdated, will most likely need some fixes"

        #define PLATFORM            PLATFORM_WINDOWS
        #define PLATFORM_STR        "Microsoft Windows"

        #define SYSTEM_WIN32        0   // Working
        #define SYSTEM_VISTA        1   //
        #define SYSTEM_WIN7         2   //
        #define SYSTEM_DEFAULT      SYSTEM_WIN32

        #define SYSTEM              SYSTEM_WIN32
        #define SYSTEM_STR          "Win32"

    // This is some unknown (undetected) system thus we don't really know how
    // to do some important things; with networking and file operations being
    // the most important parts which can't be written in a system independent
    // way.
    //   We will make a best guess and try and use the code for PLATFORM_UNIX
    // and it's SYSTEM_UNIX, which should work in most cases where no changes
    // to the code is needed.
    #else
        #warning "Unknown system: Either most stuff will work, or a lot will fail."
        #define PLATFORM        PLATFORM_UNIX
        #define PLATFORM_STR    "Unknown Platform (assuming UNIX-like)"
        #define SYSTEM          SYSTEM_UNIX
        #define SYSTEM_STR      "Unknown System (assuming UNIX-like)"

    #endif // defined(unix) || defined(_unix) || defined(__unix) || defined(__unix__)

#endif // PLATFORM

#endif // TARGET_H
