/******************************************************************************
 * file: gamelog.h
 *
 * created: 2012-02-20 by jimmy
 * authors: jimmy
 *****************************************************************************/
// TODO: Make it possible to define a logging level both during compilation and during run-time.
//       All logging levels excluded during compilation will/should result in zero code for those
//       cases. If during run-time there is a change to the log level to include those log levels
//       excluded during compilation it will have no effect (since the code isn't there).
//         This way one could conceivably add any number of very high verbosity log message even
//       for performance critical parts and have them automatically excluded in a release version.

#ifndef GAMELOG_H
#define GAMELOG_H

#include <cassert>
#include <cstdio>
#include <sys/time.h>

#include <cstring>
#include <mutex>
#include <map>



// ***************************************************************************
// START GameLog Configuration

#define GAMELOG_ENABLE_DEBUG    // Enables logging levels VERBOSE, DETAIL and DEBUG.
#define GAMELOG_ENABLE_INFO     // Enables logging levels INFO, WARNING, ALERT.
#define GAMELOG_ENABLE_ERROR    // Enables logging levels ERROR, CRITICAL and FATAL.


#define GAMELOG_ENABLE_LOG_PREFIX
#define GAMELOG_ENABLE_LOG_PREFIX_SEVERITY
//#define GAMELOG_PREFIX          "        ] "
#define GAMELOG_PREFIX          gamelog_prefix()

// FIXME: Add bounds checking for the prefix array.
extern const char* gamelog_prefix(void);

// END GameLog Configuration
// ***************************************************************************


// TODO: Proper usage of __PRETTY_FUNCTION__ vs __FUNCTION__
//       Find which versions of gcc that support __PRETTY_FUNCTION__ and use for those, the rest
//       will have to do with using __FUNCTION__ instead.
// TODO: Decide if we really want to have those prefixes for log entries above ERROR level.
//       It could easily make things too verbose and hard to read, but it is also easy to change later on
//       if it causes a problem with readability.
// NOTE: The following macros will print to the log file, unless its logging level is higher than
//       the currently set logging level for the GameLog.
//         The check for an appropriate logging level is made twice, once before calling the logging function
//       and once inside the logging function. This to ensure correct behaviour and to avoid the function call
//       for too low severity level messages.

// In debug mode we automatically enable all logging levels.
#ifdef DEBUG
    #ifndef GAMELOG_ENABLE_DEBUG
        #define GAMELOG_ENABLE_DEBUG
    #endif
    #ifndef GAMELOG_ENABLE_INFO
        #define GAMELOG_ENABLE_INFO
    #endif
    #ifndef GAMELOG_ENABLE_ERROR
        #define GAMELOG_ENABLE_ERROR
    #endif
#endif


// 3 severity levels for application debugging.
#ifdef GAMELOG_ENABLE_DEBUG
    #define log_VERBOSE(group, ...) GameLog::instance().log(GameLogAttr::VERBOSE, group, __VA_ARGS__)
    #define log_DETAIL(group, ...)  GameLog::instance().log(GameLogAttr::DETAIL, group, __VA_ARGS__)
    #define log_DEBUG(group, ...)   GameLog::instance().log(GameLogAttr::DEBUGLVL, group, __VA_ARGS__)
#else
    #define log_VERBOSE(...)
    #define log_DETAIL(...)
    #define log_DEBUG(...)
#endif

// 3 severity levels for application information logging.
#ifdef GAMELOG_ENABLE_INFO
    #define log_INFO(group, ...)    GameLog::instance().log(GameLogAttr::INFO, group, __VA_ARGS__)
    #define log_WARNING(group, ...) GameLog::instance().log(GameLogAttr::WARNING, group, __VA_ARGS__)
    #define log_ALERT(group, ...)   GameLog::instance().log(GameLogAttr::ALERT, group, __VA_ARGS__)
#else
    #define log_INFO(...)
    #define log_WARNING(...)
    #define log_ALERT(...)
#endif

// 3 severity levels for application error logging.
#ifdef GAMELOG_ENABLE_ERROR
    #define log_ERROR(group, ...)           GameLog::instance().log(GameLogAttr::ERRORLVL, group, __VA_ARGS__)
    #define log_CRITICAL(group, msg, ...)   GameLog::instance().log(GameLogAttr::CRITICAL, group, "CRITICAL @ func: %s @ file: %s:%i\n" msg, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
    #define log_FATAL(group, msg, ...)      GameLog::instance().log(GameLogAttr::FATAL, group, "FATAL @ func: %s @ file: %s:%i\n" msg, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define log_ERROR(msg, ...)
    #define log_CRITICAL(msg, ...)
    #define log_FATAL(msg, ...)
#endif


// NOTE: Adds a new log entry without any prefix, but with the timestamp.
#define log_ADD(group, ...) GameLog::instance().log(GameLogAttr::DEFAULT, group, __VA_ARGS__)

#define log_INIT()      class log_INITCLASS log_INIT_instance(__PRETTY_FUNCTION__)

#define log_INIT_OK()   log_INIT_instance.success()



// NOTE: DEBUGLVL and ERRORLVL are named like that to avoid conflicts with existing macros.
namespace GameLogAttr {
    enum Priority {VERBOSE, DETAIL, DEBUGLVL, INFO, WARNING, ALERT, ERRORLVL, CRITICAL, FATAL, ALWAYS, Undefined};
    extern const char* MsgLevelStrings[];
    const Priority DEFAULT = INFO;
}


class GameLog {

  public:
    static GameLog& instance(void);
    bool            initialize(const char* filename);
//    bool            initialize(const char* hostname, int port);
    void            close(void);

//    void log(GameLogAttr::Priority priority, const char *format, ...);
    void log(GameLogAttr::Priority priority, const char *group, const char *format, ...);

    bool                  isSeverityLogged(GameLogAttr::Priority priority);
    GameLogAttr::Priority getPriorityLevel(void);
    void                  setPriorityLevel(GameLogAttr::Priority priority);

    void                  createPriorityGroup(std::string group, GameLogAttr::Priority priority = GameLogAttr::Priority::INFO);
    GameLogAttr::Priority getGroupPriorityLevel(std::string group);
    void                  setGroupPriorityLevel(std::string group, GameLogAttr::Priority priority);

    void fillLine(char c);
    void fillPrefixedLine(char c);
    void indent(unsigned int i);

private:
    GameLog() : logfile(NULL), logLevel(GameLogAttr::Priority::Undefined) {}
    ~GameLog();

    GameLog(const GameLog&);
    GameLog& operator=(const GameLog&);

    FILE*      logfile;
    std::mutex loglock;

    GameLogAttr::Priority logLevel;

    std::map<std::string, GameLogAttr::Priority> priorityGroups;
};

inline GameLog::~GameLog() {
    close();
}

inline GameLog &GameLog::instance(void) {
    static GameLog instanceOfGameLog;
    return instanceOfGameLog;
}

inline void GameLog::fillLine(char c) {
    char msg[80];
    memset(msg, c, 79);
    msg[79] = '\n';
    fwrite(msg, 80, 1, logfile);
}

inline void GameLog::fillPrefixedLine(char c) {
    char msg[80];
    strcpy(msg, GAMELOG_PREFIX);
    std::size_t size = strlen(msg);
    if (size < 79) {
        memset(msg+size, c, 79-size);
        msg[79] = '\n';
    }
    fwrite(msg, 80, 1, logfile);
}

inline void GameLog::indent(unsigned int i ) {
    if (i == 0)
        return;
//    logstream << std::setw(2*i-1) << ' ' << std::endl;
}

inline GameLogAttr::Priority GameLog::getPriorityLevel(void) {
    return logLevel;
}

inline void GameLog::setPriorityLevel(GameLogAttr::Priority priority) {
    if (priority != GameLogAttr::Priority::Undefined && priority != GameLogAttr::Priority::ALWAYS)
        logLevel = priority;
}

inline bool GameLog::isSeverityLogged(GameLogAttr::Priority priority) {
    if (priority < logLevel)
        return false;
    return true;
}



// Since Linux apparently doesn't support gethrtime() by default we go for gettimeofday() instead.
class log_INITCLASS {
public:
    log_INITCLASS(const char* funcname);
    ~log_INITCLASS();
    void success();
private:
    bool _success;
    static const int bsize = 256;
    struct timeval start;
    char function_name[bsize];
};

inline log_INITCLASS::log_INITCLASS(const char* funcname) {
    _success = false;
    strncpy(function_name, funcname, bsize);
    gettimeofday(&start, NULL);
    GameLog::instance().fillPrefixedLine('-');
    GameLog::instance().log(GameLogAttr::Priority::ALWAYS, NULL, "INIT %s ...", function_name);
}

inline log_INITCLASS::~log_INITCLASS() {
    struct timeval end;
    gettimeofday(&end, NULL);
    unsigned long int dt = (end.tv_sec-start.tv_sec) * 1000000 + (end.tv_usec-start.tv_usec);
    if (_success == true) {
        GameLog::instance().log(GameLogAttr::Priority::ALWAYS, NULL, "OK - INIT %s ... (%lu μs)", function_name, dt);
    } else {
        GameLog::instance().log(GameLogAttr::Priority::ALWAYS, NULL, "FAIL - INIT %s ... (%lu μs)", function_name, dt);
    }
    GameLog::instance().fillPrefixedLine('-');
}

inline void log_INITCLASS::success() {
    _success = true;
}

#endif  // GAMELOG_H
