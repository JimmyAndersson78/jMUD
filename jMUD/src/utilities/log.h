#ifndef LOG_H
#define LOG_H


#include <cassert>
//#include <cstdio>
#include <sys/time.h>

#include <cstring>
#include <mutex>



// ***************************************************************************
// START sys::log Configuration
#define SYSLOG_ENABLE_DEBUG    // Enables logging levels VERBOSE, DETAIL and DEBUG.
#define SYSLOG_ENABLE_INFO     // Enables logging levels INFO, WARNING, ALERT.
#define SYSLOG_ENABLE_ERROR    // Enables logging levels ERROR, CRITICAL and FATAL.


// log_VERBOSE()
// log_INFORMATIONAL()
// log_DEBUG()
// log_NOTICE()
// log_WARNING()
// log_ALERT()
// log_ERROR()
// log_CRITICAL()
// log_FATAL()

// Anything above a log_ERROR() can be recovered from with little/no loss of
// functionality. log_ERROR() and log_CRITICAL() events might be possible to
// continue after (with severe loss of functionality, but the aim should be to
// perform as controlled shutdown as possible. log_FATAL() is indeed fatal and
// won't just log an error but also call exit(-1);


#define SYSLOG_ENABLE_LOG_PREFIX
#define SYSLOG_ENABLE_LOG_PREFIX_SEVERITY
#define SYSLOG_PREFIX          syslog_prefix()
//#define SYSLOG_PREFIX          "        ] "

extern const char* syslog_prefix(void);
// END sys::log Configuration
// ***************************************************************************


//sys::log::event::OutOfMemory(__FILE__, __LINE__, __FUNCTION__);


// In debug mode we automatically enable all logging levels.
#ifdef DEBUG
    #ifndef SYSLOG_ENABLE_DEBUG
        #define SYSLOG_ENABLE_DEBUG
    #endif
    #ifndef SYSLOG_ENABLE_INFO
        #define SYSLOG_ENABLE_INFO
    #endif
    #ifndef SYSLOG_ENABLE_ERROR
        #define SYSLOG_ENABLE_ERROR
    #endif
#endif



namespace sys {
namespace log {

enum Severity {VERBOSE, DEBUGLVL, INFO, WARNING, ERRORLVL, FATAL, ALWAYS, Undefined};
extern const char* MsgLevelStrings[];
const Severity DEFAULT = INFO;

void printLogMessage(log::Severity severity, const char* group, log::Severity groupSeverity, const char *format, ...);
void printLogEvent(sys::log::Severity severity, const char* event);
void printLine(char c, bool prefixed = false);

extern sys::log::Severity minSeverityToLog;

inline sys::log::Severity getSeverityLevel(void) {
    return sys::log::minSeverityToLog;
}

inline void setSeverityLevel(sys::log::Severity severity) {
    if (severity != sys::log::Severity::Undefined && severity != sys::log::Severity::ALWAYS)
        sys::log::minSeverityToLog = severity;
}

template <typename ... T> inline void verbose(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::VERBOSE)
        sys::log::printLogMessage(sys::log::Severity::VERBOSE, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void debug(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::DEBUGLVL)
        sys::log::printLogMessage(sys::log::Severity::DEBUGLVL, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void info(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::INFO)
        sys::log::printLogMessage(sys::log::Severity::INFO, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void add(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::DEFAULT)
        sys::log::printLogMessage(sys::log::DEFAULT, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void warning(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::WARNING)
        sys::log::printLogMessage(sys::log::Severity::WARNING, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void error(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::ERRORLVL)
        sys::log::printLogMessage(sys::log::Severity::ERRORLVL, NULL, sys::log::Severity::Undefined, format, t...);
}

template <typename ... T> inline void fatal(const char* format, T ... t) {
    if (sys::log::minSeverityToLog <= sys::log::Severity::FATAL)
        sys::log::printLogMessage(sys::log::Severity::FATAL, NULL, sys::log::Severity::Undefined, format, t...);
}

} // namespace log
} // namespace sys



#define log_CREATEGROUP(group)    \
    namespace sys { namespace log { namespace group {                                       \
    extern sys::log::Severity severity;                                                     \
    extern bool printGroupPrefix;                                                           \
    extern FILE* file;                                                                      \
                                                                                            \
    inline log::Severity getSeverityLevel(void) {                                           \
        return sys::log::group::severity;                                                   \
    }                                                                                       \
                                                                                            \
    inline void setSeverityLevel(log::Severity p) {                                         \
        if (p != sys::log::Severity::Undefined && p != sys::log::Severity::ALWAYS)          \
            sys::log::group::severity = p;                                                  \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void verbose(const char* format, T ... t) {            \
        if (sys::log::group::severity <= sys::log::Severity::VERBOSE)                       \
            sys::log::printLogMessage(sys::log::Severity::VERBOSE, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void debug(const char* format, T ... t) {              \
        if (sys::log::group::severity <= sys::log::Severity::DEBUGLVL)                      \
            sys::log::printLogMessage(sys::log::Severity::DEBUGLVL, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void info(const char* format, T ... t) {               \
        if (sys::log::group::severity <= sys::log::Severity::INFO)                          \
            sys::log::printLogMessage(sys::log::Severity::INFO, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void add(const char* format, T ... t) {                \
        if (sys::log::group::severity <= sys::log::DEFAULT)                                 \
            sys::log::printLogMessage(sys::log::DEFAULT, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void warning(const char* format, T ... t) {            \
        if (sys::log::group::severity <= sys::log::Severity::WARNING)                       \
            sys::log::printLogMessage(sys::log::Severity::WARNING, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void error(const char* format, T ... t) {              \
        if (sys::log::group::severity <= sys::log::Severity::ERRORLVL)                      \
            sys::log::printLogMessage(sys::log::Severity::ERRORLVL, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
                                                                                            \
    template <typename ... T> inline void fatal(const char* format, T ... t) {              \
        if (sys::log::group::severity <= sys::log::Severity::FATAL)                         \
            sys::log::printLogMessage(sys::log::Severity::FATAL, #group, sys::log::group::severity, format, t...); \
    }                                                                                       \
    } } }


#define log_INITGROUP(group, groupSeverity)    \
    namespace sys { namespace log { namespace group {                                       \
    sys::log::Severity severity = groupSeverity;                                            \
    bool printGroupPrefix = true;                                                           \
    FILE* file = NULL;                                                                      \
    } } }


#ifdef LFKJHLDFKHDFHFKJHFKJHFLKJDFKJD
// 2 severity levels for application debugging.
#ifdef SYSLOG_ENABLE_DEBUG
    #define log_VERBOSE(...) sys::log::verbose(__VA_ARGS__)
    #define log_DEBUG(...)   sys::log::debug(__VA_ARGS__)
#else
    #define log_VERBOSE(...)
    #define log_DEBUG(...)
#endif

// 2 severity levels for application information logging.
#ifdef SYSLOG_ENABLE_INFO
    #define log_INFO(...)    sys::log::info(__VA_ARGS__)
    #define log_WARNING(...) sys::log::warning(__VA_ARGS__)
#else
    #define log_INFO(...)
    #define log_WARNING(...)
#endif

// 2 severity levels for application error logging.
#ifdef SYSLOG_ENABLE_ERROR
    #define log_ERROR(...)      sys::log::verbose(__VA_ARGS__)
    #define log_FATAL(msg, ...) sys::log::verbose("FATAL @ func: %s @ file: %s:%i\n"msg, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define log_ERROR(msg, ...)
    #define log_FATAL(msg, ...)
#endif
#endif


#define log_ADD(...) sys::log::add(__VA_ARGS__)

#define log_INIT()      class sys::log::log_INITCLASS log_INIT_instance(__PRETTY_FUNCTION__)
//#define log_INIT()      class sys::log::log_INITCLASS log_INIT_instance(__FUNCTION__)
#define log_INIT_OK()   log_INIT_instance.success()



namespace sys {
    namespace log {

class LogWriteToFile;
class LoggingWriteToSocket; // NOT IMPLEMENTED


class LogWriter {
public:
    LogWriter() {}
    virtual ~LogWriter() {}
    virtual bool write(const char* str, std::size_t length) = 0;
private:
    LogWriter(const LogWriter&);
    LogWriter& operator=(const LogWriter&);
};


class LogWriteToFile : public LogWriter {
public:
    LogWriteToFile(const char* filename = NULL);
    virtual ~LogWriteToFile();

    bool setup(const char* filename);
    void close(void);
    virtual bool write(const char* str, std::size_t length);

private:
    LogWriteToFile(const LogWriteToFile&);
    LogWriteToFile& operator=(const LogWriteToFile&);

    FILE*         logfile;
    std::mutex    loglock;
//    log::Severity logLevel;
};

inline LogWriteToFile::LogWriteToFile(const char* filename) : logfile(NULL), loglock() {
    setup(filename);
}

inline LogWriteToFile::~LogWriteToFile() {
    close();
}

// TODO: Buffer any data that can't be written now and write it on next call, or on close().
inline bool LogWriteToFile::write(const char* str, std::size_t length) {
    loglock.lock();
    fwrite(str, length, 1, logfile);
    #ifdef DEBUG
        fflush(logfile);
    #endif // DEBUG
    loglock.unlock();
    return true;
}


class Logging  {
public:
    static Logging* instance(void);
    bool setup(const char* filename = NULL);
    void close();

    bool write(const char* str, std::size_t length);
    void fillLine(char c);
    void fillPrefixedLine(char c);

private:
    Logging();
    Logging(const char* filename);
    ~Logging();
    Logging(const Logging&);
    Logging& operator=(const Logging&);

    LogWriter* logWriter;
};

inline Logging* Logging::instance(void) {
    static Logging* instanceOfLogging = new sys::log::Logging(NULL);
    return instanceOfLogging;
}

inline Logging::Logging(const char* filename) : logWriter(NULL) {
    setup(filename);
}

inline bool Logging::setup(const char* filename) {
    if (logWriter != NULL)
        delete logWriter;
    logWriter = new sys::log::LogWriteToFile(filename);
    return true;
}

inline void Logging::close(void) {
    delete logWriter;
    logWriter = NULL;
}

inline bool Logging::write(const char* str, std::size_t length) {
    if (logWriter == NULL)
        return false;
    logWriter->write(str, length);
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

inline log_INITCLASS::log_INITCLASS(const char* funcname) :
    _success(false),
    start({0,0})
{
    _success = false;
    char temp[bsize];
    if (sscanf(funcname, "%*s %[^(]s(", temp) == 1) {
        snprintf(function_name, bsize, "%s(...)", temp);
    } else {
//        snprintf(function_name, bsize, "%s", funcname);
        strncpy(function_name, funcname, bsize);
    }
    gettimeofday(&start, NULL);
    sys::log::printLine('-', true);
    sys::log::printLogMessage(sys::log::Severity::ALWAYS, NULL, sys::log::Severity::ALWAYS, "### START %s ...", function_name);
}

inline log_INITCLASS::~log_INITCLASS() {
    struct timeval end;
    gettimeofday(&end, NULL);
    unsigned long int dt = (end.tv_sec-start.tv_sec) * 1000000 + (end.tv_usec-start.tv_usec);
    if (_success == true) {
        sys::log::printLogMessage(sys::log::Severity::ALWAYS, NULL, sys::log::Severity::ALWAYS, "### START %s ... OK (in %lu μs)", function_name, dt);
    } else {
        sys::log::printLogMessage(sys::log::Severity::ALWAYS, NULL, sys::log::Severity::ALWAYS, "### START %s ... FAILED (in %lu μs)", function_name, dt);
    }
    sys::log::printLine('-', true);
}

inline void log_INITCLASS::success() {
    _success = true;
}


} // namespace log
} // namespace sys



log_CREATEGROUP(GameServer)
log_CREATEGROUP(GameEngine)
log_CREATEGROUP(DataEngine)
log_CREATEGROUP(WorldEngine)
log_CREATEGROUP(NetworkEngine)

log_CREATEGROUP(performance)
log_CREATEGROUP(security)
log_CREATEGROUP(testing)

#endif // LOG_H
