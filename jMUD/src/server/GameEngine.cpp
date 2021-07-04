/*---------------------------------------------------------------------------*\
|* File: GameEngine.cpp                              Part of jMUD-GameEngine *|
|*                                                                           *|
|* Description: Main game loop. Starting up game systems, loading world and  *|
|*              other data, initiating communication and cleaning up when    *|
|*              shutdown. Few of these functions are performed directly in   *|
|*              this file it only makes sure that all game systems are up    *|
|*              and running, else exiting.                                   *|
|*                                                                           *|
|* Authors: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>             *|
|* Created: 2001-02-14 by Jimmy Andersson                                    *|
|* Version: 0.2.00                                                           *|
\*---------------------------------------------------------------------------*/
#include "config.h"
#include "log.h"
#include "log.h"

#include "GameEngine.h"
#include "DataEngine.h"
#include "world/WorldEngine.h"
#include "network/NetworkEngine.h"


#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fstream>


#if (PLATFORM == PLATFORM_WINDOWS)
    #include <sys/timeb.h>
#elif (PLATFORM == PLATFORM_UNIX)
    #include <sys/utsname.h>        // uname()
    #include <sys/resource.h>       // getrusage()
    #include <unistd.h>             // sysconf()

    #if (SYSTEM == SYSTEM_SUNOS)
        #include <sys/types.h>
        #include <sys/processor.h>
        #include <sys/sysconfig.h>
    #endif
    #if (SYSTEM == SYSTEM_LINUX)
        #include <sys/time.h>       // gettimeofday()
    #endif
#endif // (PLATFORM == PLATFORM_WINDOWS)


/***
 * Standard constructor for GameEngine.
 */
GameEngine::GameEngine(void) :
    runStatus(true),
    errorCode(0),
    running(false),
    booted(false),
    _cycle_count(0),
    _cycle_time({0,0}),
    time_boot(0),
    time_now(0),
    _network_io(),
    mutex_network_io(),
    _players()
{
}


/***
 * Standard destructor for GameEngine.
 */
GameEngine::~GameEngine(void) {
}


/***
 * Starts all game systems, and initializes the communication.
 */
bool GameEngine::initialize(void) {
    log_INIT();

    sys::log::NetworkEngine::debug("*** Here we go again.");

    if (booted == true) {
        sys::log::GameEngine::error("Game is already booted.");
        return false;
    }

    // INIT: random number generator
    srand(static_cast<unsigned int>(time(NULL)));
    struct timeval lBootStart = GetTime();

//    sys::log::GameEngine::add("%s", STRING_GameVersion);
    LogSystemInfo();


    // TODO: Initialize DataEngine
    // Initialize DataEngine (with custom logging level for more efficient debugging of systems).
    if (!DataEngine::instance().initialize()) {
        sys::log::GameEngine::error("Fatal error starting DataEngine. Terminating.");
        return false;
    }


    // Initialize WorldEngine (with custom logging level for more efficient debugging of systems).
    if (!WorldEngine::instance().initialize()) {
        sys::log::GameEngine::error("Fatal error starting WorldEngine. Terminating.");
        return false;
    }


    // Initialize NetworkEngine (with custom logging level for more efficient debugging of systems).
//    if (!NetworkEngine::instance().initialize(512, 5000, "192.168.0.64", "fe80::2e0:12ff:fe34:5678")) {
//    if (!NetworkEngine::instance().initialize(512, 5000, "192.168.0.64", NULL)) {
//    if (!NetworkEngine::instance().initialize(512, 5000, NULL, NULL)) {
    if (!net::NetworkEngine::instance().initialize(512, 5000, "127.0.0.1", "::1")) {
        sys::log::GameEngine::error("Fatal error starting NetworkEngine. Terminating.");
        return false;
    }

    // Add some statistics.
    LogSystemUsage();
    runStatus = true;

    struct timeval lBootEnd = GetTime();
    long int boot_s = lBootEnd.tv_sec - lBootStart.tv_sec;
    long int boot_ms = (lBootEnd.tv_usec - lBootStart.tv_usec) / 1000;
    while (boot_ms < 0) {
            boot_s--;
            boot_ms += 1000;
    }
    sys::log::GameEngine::add("  Booted in %is %ims.", boot_s, boot_ms);
    time_boot = time(NULL);
    booted = true;

    log_INIT_OK();
    return true;
}


/***
 * This function is the standard MUD shutdown function that is to be called
 * whenever the MUD is to be shutdown. Here all allocated data structures are
 * to be freed and all connections closed. The passed argument is to indicate
 * the nature of the error, if any, that caused the MUD to be shutdown. I
 * should make a list of error codes, when i have decided them first.
 */
int GameEngine::shutdown(int err) {
    log_INIT();

    runStatus = false;

    if (err != 0) {
        sys::log::GameEngine::add("Shutdown because of error: %i", err );
    }

    // Save all valid data in the correct order.

    // TODO: Shutdown GameEngine

    // TODO: Shutdown WorldEngine

    // TODO: Shutdown NetworkEngine
    net::NetworkEngine::instance().close();

    // TOOD: Shutdown DataEngine


    LogSystemUsage();

    booted = false; running = false;

    log_INIT_OK();
    return err;
}


/***
 * This is the main game loop where everything happens, indirectly though.
 * It's quite simple and works like this:
 *
 *   1) Read/send data from/to connected players, connect any new players.
 *   2) Interpret commands and continued started actions.
 *   3) Sleep until the beat is over, unless we passed the time then no sleep.
 *   4) Go to 1, unless we should shutdown.
 *
 * The loop is interrupted if either NetworkSystem or ActionHandler reports a
 * fatal error they can't handle. So far there are very few such errors that
 * can occur, and most(all) of them are network related and not possible to
 * correct/repair at this level.
 */
int GameEngine::run(void) {

    // The main run() method has to be public, so we added this check to see if
    // anything tries to run the same server twice.
    if (booted == false)
        return 0;
    if (running == true)
        return 0;
    running = true;
    sys::log::GameEngine::add("*** GAME IS STARTED ***");

    const unsigned int cycle_length = 250;
//    const unsigned int shutdown_at_cycle_count = (1000 / 250) * 60 * 240;   // =  4 hours
//    const unsigned int shutdown_at_cycle_count = (1000 / 250) * 60 * 10;      // = 10 minutes
    const unsigned int shutdown_at_cycle_count = (1000 / 250) * 30;           // = 30 seconds

    // Boot the game and if no errors are reported then we are ready to roll.
    sys::log::GameEngine::add("Entering GameLoop ...");
    unsigned int duration = shutdown_at_cycle_count * cycle_length / 1000;
    unsigned int hours = duration / 60 / 60;
    unsigned int minutes = (duration - hours *60*60) / 60;
    unsigned int seconds = duration % 60;
    sys::log::GameEngine::add("GameLoop exiting in: %u hours, %u minutes and %u seconds", hours, minutes, seconds);

    do {
//        sys::log::GameEngine::VERBOSE("tick");
        update_cycle();

        // TODO: Add player input handling.
        if (update() < 0)
            break;

        // TODO: Add world updates.

        // FIXME: Change so that we only sleep the remainder of the cycle time, if any.
        this->sleep(cycle_length);

    } while (runStatus == true && _cycle_count < shutdown_at_cycle_count);

    sys::log::GameEngine::add("Exiting GameLoop.");
    sys::log::GameEngine::add("*** GAME IS CLOSING ***");
    return shutdown(0);
}


int  GameEngine::update(void) {

    if (!_network_io.empty()) {
        mutex_network_io.lock();
        net::NetworkMessage* m;
        unsigned int nAdd = 0, nRem = 0, nDataIn = 0, nError = 0;

        sys::log::GameEngine::debug("update(): %lu players - processing %lu input messages", DataEngine::instance().GetNumPlayers(), _network_io.size());
        while (!_network_io.empty()) {
            m = _network_io.top();
            _network_io.pop();

            switch (m->type) {
            case net::MessageTypes::NewConnection:
                sys::log::GameEngine::add("Adding player connection cid = %u", m->cid);
                DataEngine::instance().AddPlayer(m->cid);
                nAdd++;
                break;
             case net::MessageTypes::Disconnection:
                sys::log::GameEngine::add("Removing player connection cid = %u", m->cid);
                DataEngine::instance().RemPlayer(m->cid);
                nRem++;
                break;
            case net::MessageTypes::DataIncoming:
                if (m->size == 0) {
                    sys::log::GameEngine::error("Received a NetworkMessage with type=DataIncoming which had a data size of 0. Should never happen.");
                }
                nDataIn++;
                break;
            case net::MessageTypes::DataOutgoing:
                sys::log::GameEngine::error("Received a NetworkMessage with type=DataOutgoing. Should never happen.");
                nError++;
                break;
            case net::MessageTypes::DNSLookup:
                break;
            }

            net::NetworkMessage::destruct(m);
        }
        mutex_network_io.unlock();
        sys::log::GameEngine::debug("  add %u players(s), rem %u player(s), recv %u input(s) (%u err)", nAdd, nRem, nDataIn, nError);
    }

    return 0;
}


/***
 * Returns a millisecond resolution timer, that usually represents the number
 * of milliseconds since boot though that is irrelevant for us.
 */
struct timeval GameEngine::GetTime(void) {
    struct timeval tv;

    #if (PLATFORM == PLATFORM_WINDOWS)
        struct _timeb tb;

        _ftime(&tb);
        tv.tv_sec = tb.time;
        tv.tv_usec = tb.millitm*1000;
    #elif (PLATFORM == PLATFORM_UNIX)
        gettimeofday(&tv, NULL);
    #endif

    return tv;
}


/***
 * Suspends the game for a number of milliseconds equal to the argument.
 */
inline void GameEngine::sleep(unsigned int mseconds) {
    #if (PLATFORM == PLATFORM_WINDOWS)
        Sleep(mseconds);
    #elif (PLATFORM == PLATFORM_UNIX)
        struct timespec req = { mseconds / 1000, (mseconds % 1000) * 1000 * 1000};  // ms -> ns
        struct timespec rem = { 0, 0};
        nanosleep(&req, &rem);
    #endif // (PLATFORM == PLATFORM_WINDOWS)
}


/***
 * Suspends the game for a number of milliseconds equal to the argument.
 */
inline void GameEngine::sleep(struct timespec req) {
    #if (PLATFORM == PLATFORM_WINDOWS)
        Sleep(t.tv_sec*1000 + t.tv_usec/1000);
    #elif (PLATFORM == PLATFORM_UNIX)
        struct timespec rem = { 0, 0};
        nanosleep(&req, &rem);
    #endif // (PLATFORM == PLATFORM_WINDOWS)
}


/***
 * Logs some information regarding the machine and system we are running on, it
 * can be turned of by undefining LOG_SYSTEMINFO.
 *   For now there will only be useful information available under Windows.
 */
// TODO: Accumulate the info in variables in the #ifdef-#elif-#else-endif cases and print it after all
//       info has been collected.
void GameEngine::LogSystemInfo(void) {
    sys::log::add("  Platform: %s, System: %s", PLATFORM_STR, SYSTEM_STR);

    bool info_mem = false, info_cpu = false;
    double mem_total = -1, mem_free = -1;
    long int cpu_num = -1, cpus_online = -1, cpu_cores = -1;

#if (PLATFORM == PLATFORM_WINDOWS)

    #ifdef _MSC_VER
    sys::log::add("  Compiler: Microsoft C/C++ v%s", _MSC_VER);
    #elif defined(__BORLANDC__)
    sys::log::add("  Compiler: Borland v%s", __BORLANDC__);
    #else
    sys::log::add("  Compiler: <unknown>");
    #endif // (_MSC_VER)

    OSVERSIONINFO* OSVersionInfo = NULL;
    ALLOC_OBJECT(OSVersionInfo, OSVERSIONINFO);
    OSVersionInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(OSVersionInfo) == 1) {
        switch (OSVersionInfo->dwPlatformId) {
        case VER_PLATFORM_WIN32s:
            sys::log::add("  OS: Windows 3.1 Version %i.%i build %i", (int)OSVersionInfo->dwMajorVersion, (int)OSVersionInfo->dwMinorVersion, (int)OSVersionInfo->dwBuildNumber); break;
        case VER_PLATFORM_WIN32_WINDOWS:
            sys::log::add("  OS: Windows 9x Version %i.%i build %i", (int)OSVersionInfo->dwMajorVersion, (int)OSVersionInfo->dwMinorVersion, (int)LOWORD(OSVersionInfo->dwBuildNumber)); break;
        case VER_PLATFORM_WIN32_NT:
            sys::log::add("  OS: Windows NT Version %i.%i build %i", (int)OSVersionInfo->dwMajorVersion, (int)OSVersionInfo->dwMinorVersion, (int)OSVersionInfo->dwBuildNumber); break;
        default:
            sys::log::add("  OS: Unknown Windows version.");
        }
    }
    FREE_OBJECT(OSVersionInfo);

    // Get host RAM info.
    MEMORYSTATUS* memStatus = NULL;
    ALLOC_OBJECT(memStatus, MEMORYSTATUS);
    memStatus->dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(memStatus);
    mem_total = (double)memStatus->dwTotalPhys;
    mem_free = (double)memStatus->dwAvailPhys;
    info_mem = true;
    FREE_OBJECT(memStatus);

    SYSTEM_INFO* systemInfo = NULL;
    ALLOC_OBJECT(systemInfo, SYSTEM_INFO);
    GetSystemInfo(systemInfo);
    cpu_num = (int)systemInfo->dwNumberOfProcessors;

    switch (systemInfo->dwProcessorType) {
        case PROCESSOR_INTEL_386:
            sys::log::add("  CPU(s): %i Intel 386 (Type %i Revision %i Level %i)", cpu_num, (int)systemInfo->dwProcessorType, (int)systemInfo->wProcessorRevision, (int)systemInfo->wProcessorLevel); break;
        case PROCESSOR_INTEL_486:
            sys::log::add("  CPU(s): %i Intel 486 (Type %i Revision %i Level %i)", cpu_num, (int)systemInfo->dwProcessorType, (int)systemInfo->wProcessorRevision, (int)systemInfo->wProcessorLevel); break;
        case PROCESSOR_INTEL_PENTIUM:
            sys::log::add("  CPU(s): %i Intel Pentium (Type %i Revision %i Level %i)", cpu_num (int)systemInfo->dwProcessorType, (int)systemInfo->wProcessorRevision, (int)systemInfo->wProcessorLevel); break;
        default:
            sys::log::add("  CPU(s): %i Unknown (Type %i Revision %i Level %i)", cpu_num, (int)systemInfo->dwProcessorType, (int)systemInfo->wProcessorRevision, (int)systemInfo->wProcessorLevel); break;
        }

    for (int i = 0; i < cpu_num; i++) {
        if (((systemInfo->dwActiveProcessorMask) & (1 << i)) != 0)
            sys::log::add("    CPU%i: active", i);
        else
            sys::log::add("    CPU%i: inactive", i);
    }
    FREE_OBJECT(systemInfo);

#elif (PLATFORM == PLATFORM_UNIX)

    #ifdef __GNUC__
    sys::log::add("  Compiler: GCC v%d.%d.%d (built on %s at %s)", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, __DATE__, __TIME__);
    #else
    sys::log::add("  Compiler: <unknown> (built on %s at %s)", __DATE__, __TIME__);
    #endif

    struct utsname sysinfo;
    if (uname(&sysinfo) < 0) {
        sys::log::add("  Failed to get system information. (UNIX)");
    } else {
        #if (SYSTEM == SYSTEM_LINUX) || (SYSTEM == SYSTEM_CYGWIN)
        sys::log::add("  OS: %s release %s", sysinfo.sysname, sysinfo.release);
        #else
        sys::log::add("  OS: %s release %s, version: %s", sysinfo.sysname, sysinfo.release, sysinfo.version);
        #endif
        sys::log::add("  Machine: %s", sysinfo.machine);
    }
    cpu_num    = sysconf(_SC_NPROCESSORS_CONF);
    cpus_online = sysconf(_SC_NPROCESSORS_ONLN);

    #if (SYSTEM == SYSTEM_LINUX)
        // Get host RAM info.
        std::fstream meminfo("/proc/meminfo", std::ios::in);
        if (meminfo.is_open() == 0) {
            sys::log::debug("could not open /proc/meminfo");
        } else {
            char read_buf[1024];
            long int mt = -1, mf = -1, mc = -1;
            while (!meminfo.eof()) {
                meminfo.getline(read_buf, 1024, '\n');
                sscanf(read_buf, "MemTotal: %li kB", &mt);
                sscanf(read_buf, "MemFree: %li kB", &mf);
                sscanf(read_buf, "Cached: %li kB", &mc);

                mem_total = static_cast<double>(mt * 1024);
                mem_free = static_cast<double>((mf + mc) * 1024);
                info_mem = true;
            }
        }
        // Get host CPU info.
        std::fstream cpuinfo("/proc/cpuinfo", std::ios::in);
        if (cpuinfo.is_open() == 0) {
            sys::log::debug("could not open /proc/cpuinfo");
        } else {
            char read_buf[1024], cpu_name[1024];
            cpu_name[0] = '\0';
            while (!cpuinfo.eof()) {
                cpuinfo.getline(read_buf, 1024, '\n');
                sscanf(read_buf, "model name      : %[^\t\n]", cpu_name);
                sscanf(read_buf, "cpu cores%*[^:]: %li", &cpu_cores);
            }
            sys::log::add("  CPU(s): %li (%li CPUs online, %li cores) %s", cpu_num, cpus_online, cpu_cores, cpu_name);
            info_cpu = true;
        }
    #elif (SYSTEM == SYSTEM_SUNOS)
        // Get host RAM info.
        long double mem_pagesize = sysconf(_SC_PAGESIZE);
        mem_total = sysconf(_SC_PHYS_PAGES) * mem_pagesize;
        mem_free = sysconf(_SC_AVPHYS_PAGES) * mem_pagesize;
        info_mem = true;

        // Get host CPU info.
        processor_info_t pinfo;
        cpu_num = sysconf(_SC_NPROCESSORS_CONF);
        // We assume that all installed CPUs are of the same type.
        processor_info(0, &pinfo);
        sys::log::add("  CPU(s): %i %s", cpu_num, pinfo.pi_processor_type);
        // Now we loop and print some information about each detected CPU.
        int i = 0;
        do {
            if (pinfo.pi_state == P_ONLINE)
                sys::log::add("    cpu%i: active (%i MHz)", i, pinfo.pi_clock);
            else
                sys::log::add("    cpu%i: inactive (%i MHz)", i, pinfo.pi_clock);
            i++;
        } while (processor_info((processorid_t)i, &pinfo) == 0);
    #endif // (SYSTEM == SYSTEM_SUNOS)

#endif // (PLATFORM == PLATFORM_WINDOWS)

    // Did we find any information regarding the CPU(s)?
    if (info_cpu == false) {
        sys::log::add("  CPU: information unavailable");
    }

    // Did we find any information regarding the memory?
    if (info_mem == true) {
    double mem_load = 100-(mem_free / mem_total * 100.0);
    mem_total = mem_total / (1024.0 * 1024.0);
    mem_free = mem_free / (1024.0 * 1024.0);
        sys::log::add("  RAM: total: %.0fMiB, free: %.0fMiB, load: %.1f%%", mem_total, mem_free, mem_load);
    } else {
        sys::log::add("  RAM: information unavailable");
    }
}


/***
 * Logs some usefull gamestatistics.
 */
void GameEngine::LogSystemUsage(void) {

#ifdef DEBUG
    #if (PLATFORM == PLATFORM_WINDOWS)
        DWORD lMinimumWorkingSetSize, lMaximumWorkingSetSize;
        int min, max;
        if (GetProcessWorkingSetSize(GetCurrentProcess(), &lMinimumWorkingSetSize, &lMaximumWorkingSetSize) != 0) {
            min = lMinimumWorkingSetSize;
            max = lMaximumWorkingSetSize;
            sys::log::debug("  WorkingSetSize: min=%i bytes, max=%i bytes.", min, max);
        } else {
            LPVOID lpMsgBuf;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL
            );
            sys::log::debug("  WorkingSetSize: Failed to get info (%s).", lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
    #elif (PLATFORM == PLATFORM_UNIX)
//        long long mem_pagesize = sysconf(_SC_PAGESIZE);

        struct rusage rs;
        if (getrusage(RUSAGE_SELF, &rs) == 0) {
            double dt = static_cast<double>(time(NULL) - time_boot);
            double cpu_time_sys  = static_cast<double>(rs.ru_stime.tv_sec * 1000000 + rs.ru_stime.tv_usec) / 1000000;
            double cpu_time_user = static_cast<double>(rs.ru_utime.tv_sec * 1000000 + rs.ru_utime.tv_usec) / 1000000;
            double cpu_time = cpu_time_sys + cpu_time_user;
            double cpu_utilization = (cpu_time) / dt * 100;

            sys::log::add("system resource usage:");
            sys::log::add("  CPU: %fs (%f%%) (sys: %fs, user: %fs)",
                    cpu_time, cpu_utilization, cpu_time_sys, cpu_time_user);
            sys::log::add("  RAM: maxrss: %i KiB", rs.ru_maxrss/1024);
        } else {
            sys::log::add("system resource usage: <not available>");
        }
    #endif //  (PLATFORM == PLATFORM_UNIX)
#endif // DEBUG

//    sys::log::add("SYSLOG: MemoryUsage: ??? bytes.");
}
