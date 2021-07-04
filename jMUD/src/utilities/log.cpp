// TODO: Add a network logging class where everything is sent through a socket to a (remote) host.

#include "log.h"
#include <cstdarg>


log_INITGROUP(GameServer, sys::log::Severity::VERBOSE)
log_INITGROUP(GameEngine, sys::log::Severity::VERBOSE)
log_INITGROUP(DataEngine, sys::log::Severity::VERBOSE)
log_INITGROUP(WorldEngine, sys::log::Severity::INFO)
log_INITGROUP(NetworkEngine, sys::log::Severity::INFO)

log_INITGROUP(performance, sys::log::Severity::VERBOSE)
log_INITGROUP(security, sys::log::Severity::VERBOSE)
log_INITGROUP(testing, sys::log::Severity::VERBOSE)


// ***************************************************************************
// START sys::log Configuration
#include "config.h"
#include "../server/GameEngine.h"

// TODO: If the cycle count is 0 (i.e. the game hasn't started yet) use the current time instead.
const char* syslog_prefix(void) {
    const std::size_t size = 256;
    static char prefix[size];

    uint64_t cycle_count = GameEngine::instance().GetCycleCount();
    if (cycle_count != 0) {
        snprintf(prefix, size, "%10lu ] ", cycle_count);
    } else {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        snprintf(prefix, size, "%10lu ] ", cycle_count);
    }
    return prefix;
}
// END sys::log Configuration
// ***************************************************************************



namespace sys {
namespace log {

const char* MsgLevelStrings[] = {NULL, NULL, NULL, "WARNING", "ERROR", NULL, NULL, NULL};

sys::log::Severity minSeverityToLog = sys::log::Severity::INFO;


// FIXME: Create Path to Log File
//        If the path to the log file doesn't exist create it.
bool LogWriteToFile::setup(const char* filename) {
    loglock.lock();

    // FIXME: Logger "reused" with non-NULL logfile
    if (logfile != NULL) {
        char  logFooter[1024], time_str[64];
        std::time_t t = std::time(NULL);
        std::strftime(time_str, 63, "%F %T", std::localtime(&t));
        snprintf(logFooter, 1024, "###############################################################################\nsys::log: logging disabled\n  closed at = %s\n###############################################################################\n", time_str);

        fwrite(logFooter, strlen(logFooter), 1, logfile);
        fflush(logfile);
        fclose(logfile);
        logfile = NULL;
    }

    // If no file name is given default to stdout, otherwise open the given file.
    if (filename == NULL) {
        logfile = stdout;
        filename = "\"stdout\"";
    } else {
        logfile = fopen(filename, "w");
    }

    if (logfile == NULL) {
        perror("sys::log: Error while opening log output destination.");
        loglock.unlock();
        return false;
    }

    sys::log::minSeverityToLog = sys::log::Severity::INFO;
    char  logHeader[1024], time_str[64];
    std::time_t t = std::time(NULL);
    std::strftime(time_str, 63, "%F %T", std::localtime(&t));

    int result = snprintf(logHeader, 1023, "###############################################################################\nsys::log: logging enabled\n  to file    = %s\n  started at = %s\n###############################################################################\n", filename, time_str);
    if (result < 0 || result > 1023) {
        perror("sys::log: Error while formatting log header.");
        loglock.unlock();
        return false;
    }

    fwrite(logHeader, result, 1, logfile);

    loglock.unlock();
    return true;
}


void LogWriteToFile::close(void) {
    char  logFooter[1024], time_str[64];
    std::time_t t = std::time(NULL);
    std::strftime(time_str, 63, "%F %T", std::localtime(&t));
    snprintf(logFooter, 1024, "###############################################################################\nsys::log: logging disabled\n  closed at = %s\n###############################################################################\n", time_str);

    loglock.lock();
    fwrite(logFooter, strlen(logFooter), 1, logfile);
    fflush(logfile);
    fclose(logfile);
    loglock.unlock();
}



void printLogMessage(sys::log::Severity severity, const char* group, sys::log::Severity groupSeverity, const char *format, ...) {
    assert(format != NULL);
    assert(severity != sys::log::Severity::Undefined);

    if ((group != NULL) && severity < groupSeverity) {
        return;
    } else if (group == NULL && severity < sys::log::minSeverityToLog) {
        return;
    }

    const std::size_t size = 4096;
    std::size_t length = 0;
    char logMessage[size];

    const char* prefix = SYSLOG_PREFIX;

    // TODO: Check the return value from the snprintf() commands and handle it.
    if (prefix != NULL && prefix[0] != '\0') {
        length += snprintf(logMessage, size-length, "%s", prefix);
    }
    #ifdef SYSLOG_ENABLE_LOG_PREFIX_SEVERITY
        length+= snprintf(&(logMessage[length]), size-length, "(%i) ", severity);
    #endif
    if (group != NULL) {
        length += snprintf(&(logMessage[length]), size-length, "%s: ", group);
    }
    if (sys::log::MsgLevelStrings[severity] != NULL) {
        length += snprintf(&(logMessage[length]), size-length, "%s: ", sys::log::MsgLevelStrings[severity]);
    }

    // Output the formatted message into the buffer, starting right after the prefix.
    va_list args;
    va_start(args, format);
    int result = vsnprintf(&(logMessage[length]), size-length, format, args);
    va_end(args);

    // TODO: If the output buffer is too small create a larger one and try again, but don't loop it.
    if (result < 0 || static_cast<std::size_t>(result) > (size-length)) {
        perror("GameLog: Error while formatting output message.");
    }
    length += result;

    // Add a newline if there is room for it.
    if (length < size) {
        logMessage[length] = '\n';
        length++;
        logMessage[length] = '\0';
    }

    // TODO: Verify that everything got written to the destination, if not save the remainder until the
    //       next call to log something.
    // Write the prepared output to a file.
    //    std::size_t fwrite ( const void * ptr, std::size_t size, std::size_t count, FILE * stream );
    sys::log::Logging::instance()->write(logMessage, length);
}


void printLine(char c, bool prefixed) {
    char line[80];
    std::size_t length = 0;
    if (prefixed == true) {
        strcpy(line, SYSLOG_PREFIX);
        length = strlen(line);
    }
    if (length < 79)
        memset(line+length, c, 79-length);
    line[79] = '\n';
    sys::log::Logging::instance()->write(line, 80);

}


} // namespace log
} // namespace sys
