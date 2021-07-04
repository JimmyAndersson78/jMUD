/******************************************************************************
 * file: debug.h
 *
 * description: Macros and functions for debugging purposes.
 *
 * created: 2012-02-19 by jimmy
 * authors: jimmy
 *****************************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

#if defined(DEBUG) || defined(_DEBUG)
    #ifndef DEBUG
        #define DEBUG
    #endif

    // Debug Specific Memory Operations
    #undef  DEBUG_MEMORY
    #undef  DEBUG_NULLWARNING
    #undef  DEBUG_CHECKHEAP
    #undef  DEBUG_MEMCOPY

#else
    #ifndef NDEBUG
        #define NDEBUG
    #endif
#endif



#ifdef USE_CUSTOM_ASSERT
    #include <iostream>

    #define ASSERT(condition, string) \
        if ((condition) == false) { \
            std::cerr << "Assertion failed: '" << #condition << "' at '" << __FILE__ \
                      << ":" << __LINE__ << "'" << std::endl \
                      << "\"" << string << "\"" << std::endl; \
        }
    #else
    #define ASSERT(condition, string)
#endif /* USE_CUSTOM_ASSERT */


#endif // DEBUG_H
