/******************************************************************************
 * file: gamelog.cpp
 *
 * created: 2012-02-25 by jimmy
 * authors: jimmy
 *****************************************************************************/
// TODO: Add a network logging option where everything is sent through a socket to a (remote) host.

#include "gamelog.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <cstring>
#include <cstdarg>


// ***************************************************************************
// START GameLog Configuration
#include "config.h"
#include "../server/GameEngine.h"

// FIXME: Add bounds checking for the prefix array.
const char* gamelog_prefix(void) {
    static char prefix_dynamic[256];

    uint64_t cycle_count = GameEngine::instance().GetCycleCount();
//    if (cycle_count != 0) {
        sprintf(prefix_dynamic, "%10lu ] ", cycle_count);
/*    } else {
        struct timeval tv;
        gettimeofday(&tv, NULL);


    }*/
    return prefix_dynamic;

}
// END GameLog Configuration
// ***************************************************************************


namespace GameLogAttr {
const char* MsgLevelStrings[] = {NULL, NULL, NULL, NULL, NULL, "WARNING", "ALERT", "ERROR", NULL, NULL, NULL };
}

// FIXME: Create Path to Log File
//        If the path to the log file doesn't exist create it.
bool GameLog::initialize(const char* filename) {
    assert(logfile == NULL);
    assert(logLevel == GameLogAttr::Priority::Undefined);

    // If no file name is given default to stdout, otherwise open the given file.
    if (filename == NULL) {
        logfile = stdout;
        filename = "\"stdout\"";
    } else {
        logfile = fopen(filename, "a");
    }

    if (logfile == NULL) {
        perror("GameLog: Error while opening log output destination.");
        return false;
    }
    logLevel = GameLogAttr::Priority::VERBOSE;

    fillLine('#');
    char  msg[1024], time_str[64];

    std::time_t t = std::time(NULL);
    std::strftime(time_str, 63, "%F %T", std::localtime(&t));

    int result = snprintf(msg, 1023, "GameLog: logging enabled\n  to file    = %s\n  started at = %s\n", filename, time_str);

    if (result < 0 || result > 1023) {
        perror("GameLog: Error while formatting output header.");
        return false;
    }

    fwrite(msg, result, 1, logfile);
    fillLine('#');

    return true;
}

void GameLog::close(void) {
    loglock.lock();
    logLevel = GameLogAttr::Undefined;
    char msg[] = "GameLog: closing down\n";

    fillLine('#');
    fwrite(msg, strlen(msg), 1, logfile);
    fillLine('#');

    fflush(logfile);
    fclose(logfile);
    loglock.unlock();
}


// NOTE: It is possible that between checking for the appropriate logging level and aqcuiring the loglock
//       the logging level is changed, but that is ok since it was ok when we decided to perform the logging.
//void GameLog::log(const char *group, GameLogAttr::Priority priority, const char *format, ...) {
void GameLog::log(GameLogAttr::Priority priority, const char *group, const char *format, ...) {
    assert(format != NULL);
    assert(priority != GameLogAttr::Priority::Undefined);

    GameLogAttr::Priority efficientPriority = logLevel;
    if (group != NULL) {
        efficientPriority = getGroupPriorityLevel(group);
        if (efficientPriority == GameLogAttr::Priority::Undefined)
            efficientPriority = logLevel;
    }
    if (priority < efficientPriority)
        return;
    loglock.lock();

    const std::size_t size = 1024 * 32;
    std::size_t length = 0;
    char msg[size];

    const char* prefix = GAMELOG_PREFIX;

    // TODO: Check the return value from the snprintf() commands and handle it.
    if (prefix != NULL && prefix[0] != '\0') {
        length += snprintf(msg, size-length, "%s", prefix);
    }
    #ifdef GAMELOG_ENABLE_LOG_PREFIX_SEVERITY
        length+= snprintf(&(msg[length]), size-length, "(%i) ", priority);
    #endif
    if (group != NULL) {
        length += snprintf(&(msg[length]), size-length, "%s: ", group);
    }
    if (GameLogAttr::MsgLevelStrings[priority] != NULL) {
        length += snprintf(&(msg[length]), size-length, "%s: ", GameLogAttr::MsgLevelStrings[priority]);
    }

    // Output the formatted message into the buffer, starting right after the prefix.
    va_list args;
    va_start(args, format);
    int result = vsnprintf(&(msg[length]), size-length, format, args);
    va_end(args);

    // TODO: If the output buffer is too small create a larger one and try again, but don't loop it.
    if (result < 0 || static_cast<size_t>(result) > (size-length)) {
        perror("GameLog: Error while formatting output message.");
    }
    length += result;

    // Add a newline if there is room for it.
    if (length < size) {
        msg[length] = '\n';
        length++;
        msg[length] = '\0';
    }

    // TODO: Verify that everything got written to the destination, if not save the remainder until the
    //       next call to log something.
    // Write the prepared output to a file.
    //    std::size_t fwrite ( const void * ptr, std::size_t size, std::size_t count, FILE * stream );
    fwrite(msg, length, 1, logfile);

    // Flush the output in debug mode, otherwise let it be done whenever deemed more efficient.
    #ifdef DEBUG
        fflush(logfile);
    #endif // DEBUG
    loglock.unlock();
}


void GameLog::createPriorityGroup(std::string group, GameLogAttr::Priority priority) {
    priorityGroups.insert(std::pair<std::string, GameLogAttr::Priority>(group, priority));
}


void GameLog::setGroupPriorityLevel(std::string group, GameLogAttr::Priority priority) {
    priorityGroups.insert(std::pair<std::string, GameLogAttr::Priority>(group, priority));
}


GameLogAttr::Priority GameLog::getGroupPriorityLevel(std::string group) {
    std::map<std::string, GameLogAttr::Priority>::iterator i = priorityGroups.find(group);
    if (i == priorityGroups.end()) {
        return GameLogAttr::Priority::Undefined;
    }
    return (*i).second;
}



