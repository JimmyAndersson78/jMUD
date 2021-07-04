#include "config.h"
#include "GameServer.h"
#include "log.h"
#include "GameEngine.h"

#include <iostream>     // std::cout
#include <cstring>      // strcmp()
#include <signal.h>     // signal()



void signal_handler(int sig);
void signal_handler_init();



GameServer::GameServer() {
}


GameServer::~GameServer() {
}


int GameServer::run(int argc, char *argv[]) {

    // Initialize the logging system and set its logged priority level.
    sys::log::setSeverityLevel(sys::log::Severity::VERBOSE); // Default min severity level to print.

    sys::log::GameServer::setSeverityLevel(sys::log::Severity::VERBOSE);
    sys::log::GameEngine::setSeverityLevel(sys::log::Severity::VERBOSE);
    sys::log::DataEngine::setSeverityLevel(sys::log::Severity::VERBOSE);
    sys::log::WorldEngine::setSeverityLevel(sys::log::Severity::INFO);
    sys::log::NetworkEngine::setSeverityLevel(sys::log::Severity::INFO);

    sys::log::performance::setSeverityLevel(sys::log::Severity::VERBOSE);
    sys::log::security::setSeverityLevel(sys::log::Severity::VERBOSE);
    sys::log::testing::setSeverityLevel(sys::log::Severity::VERBOSE);


    if (argc > 1) {
        for (int i = 1; i < argc; i++ ) {
            if ((strcmp( argv[i], "-h") == 0) || (strcmp( argv[i], "--help") == 0)) {
                printHelp();
                return 0;
            } else if ((strcmp( argv[i], "-r") == 0) || (strcmp( argv[i], "--run") == 0)) {
//                return -1;
            } else {
                std::cout << "Unknown argument: '" << argv[i] << "'" << std::endl;
                return -1;
            }
        }
    }


    if (initialize() == false)
        return -1;

    sys::log::GameServer::verbose("Running the game.");
    int rval = GameEngine::instance().run();

    if (shutdown() == false)
        return -1;

    return rval;
}


bool GameServer::initialize() {
    sys::log::add("Registering signal handler(s).");
    signal_handler_init();

    sys::log::GameServer::verbose("Initializing the game.");
    try {
        if (GameEngine::instance().initialize() == false) {
            return false;
        }
    } catch(std::exception &e){
       std::cout << e.what() << std::endl;
       return false;
    }

    return true;
}


bool GameServer::shutdown() {
    // TODO: Make flushing and closing of log file automatic... or not since then we can't guarantee when
    //       it is destructed... or does that matter since we are explicitly shutting down things before
    //       exiting so we should be able to safely let the logging system auto-shutdown.
    sys::log::Logging::instance()->close(); // Close the log file (if any).

    return true;
}


void GameServer::printHelp(void) {
    std::cout << "jMUD v0.0.1-alpha" << std::endl << std::endl;
    std::cout << " Usage: jmud [options]" << std::endl << std::endl;
    std::cout << " Options:" << std::endl;
    std::cout << "  -h, --help   Displays this helptext and doesn't boot the MUD." << std::endl;
    std::cout << "  -r, --run    Runs the MUD server." << std::endl;
}



void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
            GameEngine::instance().shutdown(SIGINT);
            break;

        default:
            std::cerr << "ERROR: Unhandled signal " << sig << std::endl;
            break;
    }
}


void signal_handler_init() {
    // SIG_IGN is defined using a C-style cast, so we disable -Wold-style-cast warning while using it.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    // Ignore SIGPIPE signals, they will be handled directly in the network code instead.
    signal(SIGPIPE, SIG_IGN);
    #pragma GCC diagnostic pop

    // Shutdown cleanly on interrupt.
    signal(SIGINT, signal_handler);
}
