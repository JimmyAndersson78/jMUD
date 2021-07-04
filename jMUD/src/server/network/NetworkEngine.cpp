/******************************************************************************
 * file: NetworkEngine.cpp
 *
 * created: 2012-02-20 by jimmy
 * authors: jimmy
 *****************************************************************************/
// TODO: De-couple NetworkEngine
//       Make all all interaction with other parts go through call-back functions that are registered on
//       initialization. Call-backs are as follows:
//       1) New connection accepted and connection closed reporting.
//       2) Incomming data accepted.
//       3) Outgoing data added.
//       4) DNS lookup finished.
//
//         NetworkEngine should only interact with "the outside" through initialization given call-backs
//       for passing network messages (I/O data, new/closed connections, ...), and while doing so each
//       connection is refered to with a run-time unique ID.
//       * If some layer receives a message related to an ID that it doesn't recognize it should just ignore
//         that message, except if it is a message stating that a new connection was made, after appropriate
//         logging.
//
//
//       Threads: How many threads should we use?
//
//                *) accept thread(s) (IPv4 + IPv6 => 2)
//                *) recv thread(s) (one or one each for every 128? 256? connections? => 1...)
//                *) send/disconnect thread(s) (one => 1)
//
//                So a minimum of 3 threads (no IPv6 support and no "unlimited" scaling of allowed
//                connections) up to 3 + 1 per  256-connected-users (ie 4 in any reasonable case). I really
//                like the idea of automatically scaling past the FD_SETSIZE in a simple and elegant way.
//
//                When to spawn a recv-thread?
//                * Whenever the average recv-thread utilization goes above 95% (?) a new recv-thread is
//                  spawned and put into the regular recv-thread list. Checking and spawning should be done
//                  by the recv-threads themselves whenever they check for new connections.
//
//                When to terminate a recv-thread?
//                * Whenever the recv-thread utilization rate drops below 75% (?) the last thread (which
//                  presumably has the least amount of connections) is halted and its connections distributed
//                  among the other threads (in order). If any connections remain then the canibalized thread
//                  will continue running, if not it will be terminated.
//
//                How to distribute work?
//                * The accept thread(s) will put all incoming new connections in a list and all non-full
//                  recv-threads will poll that list periodically to check for new connections and if any
//                  take over as many as they can manage.
//
//                socketpair(AF_UNIX, SOCK_STREAM, 0, sv): One pair for each accept-thread, so we can wake
//                  them up from a blocking select call and make them process global state changes.
//

// ***** Priority: High *****

// FIXME: Add exception handling for object allocation.
// TODO: Add IPv6 support. *DONE*

// TODO: Add a call-back function argument to NetworkEngine::initialize(), so we can pass along input to
//       the game.

// TODO: Properly and comprehensively handle all return values from all system calls.

// TODO: Move all configurable values to constants in the NetworkEngine class.

// TODO: Code cleanup/clarification/simplification/comprehensiveness.

// TODO: Stress test epoll() based polling. *DONE*
// TODO: Stress test select() based polling. *DONE*
// TODO: Concurrent connections limit *DONE*
//       Based on FD_SETSIZE or sysconf() depending on if select() or epoll() based multiplexing is used.



// ***** Priority: Low *****

// Logging Related
// TODO: Add pre-logging for most/all system calls.
//       For example all socket_xxx functions should log what they intend to do before doing so and
//       afterwards logging the result.
// TODO: Consistent formatting for logging messages.
// TODO: Add more status logging.
// TODO: All output of error messages from system calls should include both value and string. *DONE*

// Platform Related
// TODO: Complete all #if (PLATFORM == PLATFORM_XXX) checks with default cases.
// TODO: Minimize and comment header file includes.

// TODO: Alter all socket_mode_xxx() methods to take an enable/disable argument?
//       I don't really see a usecase for us where that could be useful, so wait with that for now.

// TODO: Test so that nothing is bound to the port we are trying to bind to before doing so.

// TODO: Overhaul, extend and "complete" the statistics gathering/logging functionality.
//       Count successful reads/writes, blocked reads/writes, failed reads/writes, ...
// TODO: Write out byte counts in the format "RX: xxxxxxxxx bytes (yyy ?iB)", where the unit for the binary
//       prefix is dynamically decided so that yyy never is longer than 3 characters.
// TODO: Analyse and log buffer usage levels in DEBUG mode, so we can judge if we use buffers of
//       adequate/reasonable sizes.


#include "config.h"
#include "log.h"
#include "NetworkEngine.h"
#include "NetworkEngineAccept.h"
#include "NetworkEngineSend.h"
#include "NetworkEngineRecv.h"
#include "UnorderedArray.h"
#include "../GameEngine.h"


#if (PLATFORM == PLATFORM_UNIX)
    #include <arpa/inet.h>      // inet_addr()
    #include <fcntl.h>          // fcntl()
    #include <netdb.h>          // getnameinfo(), NI_MAXHOST
    #include <netinet/tcp.h>    // TCP_NODELAY

//    #include <sys/types.h>
//    #include <sys/stat.h>
//    #include <sys/socket.h>
//    #include <netinet/in.h>
//    #include <inttypes.h>
#endif

#include <cassert>


namespace net {


uint64_t NetworkEngine::rx_bytes;
uint64_t NetworkEngine::tx_bytes;
uint64_t NetworkEngine::nsocket_recv;
uint64_t NetworkEngine::nsocket_send;
uint32_t NetworkEngine::nsocket_accept;


/***
 * Constructor for the communications class. Initializes all pointers to NULL,
 * and tries to allocate memory for those that should be done for.
 */
NetworkEngine::NetworkEngine(void):
    threads(0),
    mutex_threads(),
    server_hostname(NULL),
    server_port(4000),
    _shutdown(false),
    _terminate(false),
    nextCID(1),
    users_total(0),
    users_current(0),
    users_peak(0),
    _MaxConnectionsTotal(256),
    uqueue_new(),
    uqueue_remove(),
    messagesToSend(),
    threads_accept(0),
    threads_recv(0),
    threads_send(0)
{
}


NetworkEngine::~NetworkEngine(void) {
}


/**
 * Initializes the communication system(s).
 */
bool NetworkEngine::initialize(int maxConnections = 256, IPPort port = 4000, const char *ipv4 = NULL, const char *ipv6 = NULL) {
    log_INIT();
    sys::log::NetworkEngine::debug("initialize: IPv4 server ip = %s, port = %lu", ipv4, port);
    sys::log::NetworkEngine::debug("initialize: IPv6 server ip = %s, port = %lu", ipv6, port);

    if (ipv4 == NULL && ipv6 == NULL) {
        sys::log::NetworkEngine::warning("Trying to bind to all IP addresses for both IPv4 and IPv6, one will fail.");
    } else if (ipv4 == NULL || ipv6 == NULL) {
        sys::log::NetworkEngine::warning("Binding to an unspecified IP for IPv4 or IPv6, depending on bind order this may fail.");
    }

    server_port = port;
    // TODO: Should we treat an argument of 0 (aka random port) as an error?
    if (server_port == 0) {
        sys::log::NetworkEngine::error("Makes no sense to listen to a random port. Aborting.");
        return false;
    }
    if (server_port < 1024) {
        int p = server_port;
        sys::log::NetworkEngine::warning("Using a reserved port (<1024). (port=%i)", p);
    }

    // Will correct the assumption for max number of open file descriptors
    // later, if we can get any information regarding it from the system.
    int max_sys = 256, max_init = maxConnections, max_setsize = FD_SETSIZE;

    #if (PLATFORM == PLATFORM_WINDOWS)
        // NOTE: Windows require Winsock to be activated to make network sockets available.
        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = 0x0202;
        if (WSAStartup(wVersionRequested, &wsaData) != 0) {
            wVersionRequested = 0x0101;
            if (WSAStartup(wVersionRequested, &wsaData) != 0) {
                sys::log::FATAL: WinSock 1.1 is not available. (%i::FATAL(getErrorCode());
                return false;
            }
            max_sys = wsaData.iMaxSockets;
        }
        int avh = LOBYTE(wsaData.wHighVersion);
        int avl = HIBYTE(wsaData.wHighVersion);
        int vl = LOBYTE(wsaData.wVersion);
        int vh = HIBYTE(wsaData.wVersion);
        sys::log::NetworkEngine::add("  WinSock v%i.%i available, using WinSock v%i.%i", avh, avl, vh, vl);
        sys::log::NetworkEngine::debug("  WinSock Description: \"%s\"", wsaData.szDescription);
        sys::log::NetworkEngine::debug("  WinSock SystemStatus: \"%s\"", wsaData.szSystemStatus);
    #elif (PLATFORM == PLATFORM_UNIX)
        max_sys = static_cast<int>(sysconf(_SC_OPEN_MAX));
    #endif // (PLATFORM == PLATFORM_WINDOWS)

    // Determine the maximum number of simultaneous connections we could and should support.
    sys::log::NetworkEngine::debug("Deciding how many connections we will allow...");
    sys::log::NetworkEngine::debug("  FD_SETSIZE   = %4i", max_setsize);
    sys::log::NetworkEngine::debug("  initialize   = %4i", max_init);
    sys::log::NetworkEngine::debug("  system max   = %4i", max_sys);

    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        if (FD_SETSIZE > 64) {
            _MaxConnectionsTotal = FD_SETSIZE - 32;
        } else {
            _MaxConnectionsTotal = FD_SETSIZE - 8;
        }
    #elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
        _MaxConnectionsTotal = max_sys - 32;
    #endif

    users_total = users_current = users_peak = 0;
    sys::log::NetworkEngine::add("max connections = %i", _MaxConnectionsTotal);


    sys::log::NetworkEngine::debug("Logging host network related information.");
    sys::log::NetworkEngine::add("Host Network Information:");
    server_hostname = new char[NI_MAXHOST];
    if (gethostname(server_hostname, SIZE_MaxBufferSize) == 0) {
        sys::log::NetworkEngine::add("  hostname: %s", server_hostname);
    } else {
        sys::log::NetworkEngine::add("  hostname: <unknown>", server_hostname);
        sys::log::NetworkEngine::warning("Error getting hostname for the running host. (%i:%s)", get_error_code(), get_error_msg());
    }
    // TODO: Log all IPv4 and IPv6 addresses associated with the host.


    sys::log::NetworkEngine::debug("Spawning server accept-threads...");
    SpawnAcceptThread("IPv4", ipv4, server_port, AF_INET);
    SpawnAcceptThread("IPv6", ipv6, server_port, AF_INET6);
    sys::log::NetworkEngine::debug("  %3i accept-threads spawned", threads_accept);
    if (threads_accept == 0) {
        sys::log::NetworkEngine::error("Failed to start any accept-threads. Aborting.");
        close();
        return false;
    }

    sys::log::NetworkEngine::debug("Spawning server recv-threads...");
    SpawnRecvThread("Recv1");
    sys::log::NetworkEngine::debug("  %3i recv-threads spawned", threads_recv);
    if (threads_recv == 0) {
        sys::log::NetworkEngine::error("Failed to start any recv-threads. Aborting.");
        close();
        return false;
    }

    sys::log::NetworkEngine::debug("Spawning server cleanup-threads...");
    SpawnSendThread("Send1");
    sys::log::NetworkEngine::debug("  %3i send-threads spawned", threads_send);
    if (threads_send == 0) {
        sys::log::NetworkEngine::error("Failed to start any send-threads. Aborting.");
        close();
        return false;
    }

    log_INIT_OK();
    return true;
}


/***
 * Closes the communications system, and deallocates the memory allocated for
 * the datastructures used in the NetworkEngine class.
 */
bool NetworkEngine::close(void) {
    // NOTE: This will signal all threads to terminate, except for the NetworkEngineSend threads which we
    //       want to stay running to properly close down all connections.
    _shutdown = true;

    // TODO: Add proper mechanisms to allow for interrupting the select wait in the accept threads so we
    //       can wait for them to exit properly before closing down NetworkEngine properly.
    //         Create a pipe(2) to each thread, this could possibly be used for passing command messages
    //       in the future.

    // Wait for all threads to terminate.
//    for (SocketThread* t: threads) {
        // TOOD: Uncomment when all SocketThread classes have been made to terminate properly.
        // t->join();
//    }

    //
    sleep(2);

    // NOTE: This will signal all NetworkEngineSend threads to terminate.
    _terminate = true;

    #if (PLATFORM == PLATFORM_WINDOWS)
        // NOTE: Windows wants us to report when we don't need sockets anymore.
        WSACleanup();
    #endif // (PLATFORM == PLATFORM_WINDOWS)

    sleep(4);

    LogStatus();
    return true;
}


void NetworkEngine::SpawnAcceptThread(const char* name, const char* addr, IPPort port, int type) {

    NetworkEngineAccept* t = new NetworkEngineAccept(name, addr, port, type);
    if (t == NULL) {
        sys::log::NetworkEngine::error("Failed to spawn accept-thread '%s'.", name);
        return;
    }

    if (t->run() == false) {
        delete t;
        sys::log::NetworkEngine::error("Failed to run accept-thread '%s'.", name);
        return;
    }

    mutex_threads.lock();
    ++threads_accept;
    threads.push_back(t);
    mutex_threads.unlock();

    sys::log::NetworkEngine::add("Spawned accept-thread <%s> (IP=%s, port=%hu)", name, addr, port);
}


void NetworkEngine::SpawnRecvThread(const char* name) {
    NetworkEngineRecv* t = new NetworkEngineRecv(name);
    if (t == NULL) {
        sys::log::NetworkEngine::error("Failed to spawn recv-thread '%s'.", name);
        return;
    }

    if (t->run() == false) {
        delete t;
        sys::log::NetworkEngine::error("Failed to run recv-thread '%s'.", name);
        return;
    }

    mutex_threads.lock();
    ++threads_recv;
    threads.push_back(t);
    mutex_threads.unlock();

    sys::log::NetworkEngine::add("Spawned recv-thread <%s>", name);
 }


void NetworkEngine::SpawnSendThread(const char* name) {
    NetworkEngineSend* t = new NetworkEngineSend(name);
    if (t == NULL) {
        sys::log::NetworkEngine::error("Failed to spawn send-thread '%s'.", name);
        return;
    }

    if (t->run() == false) {
        delete t;
        sys::log::NetworkEngine::error("Failed to run send-thread '%s'.", name);
        return;
    }

    mutex_threads.lock();
    ++threads_send;
    threads.push_back(t);
    mutex_threads.unlock();

    sys::log::NetworkEngine::add("Spawned send-thread <%s>", name);
 }


SOCKET NetworkEngine::setup_server_socket(int type, const char* host, IPPort port) {
    assert(((type == AF_INET) || (type == AF_INET6)));
    assert(port != 0);

    SOCKET server = INVALID_SOCKET;

    if (host != NULL && host[0] == '\0') {
        sys::log::NetworkEngine::warning("Empty string as address to bind to, binding to all instead.");
        host = NULL;
    }

    if ((server = socket_create(type)) == INVALID_SOCKET) {
        sys::log::NetworkEngine::fatal("Unable to create server socket. Aborting.");
        return INVALID_SOCKET;
    }

    if (socket_mode_reuseaddr(server) == false) {
        sys::log::NetworkEngine::warning("Unable to properly set options on server socket.");
    }

    if (socket_mode_linger(server) == false) {
        sys::log::NetworkEngine::warning("Unable to properly set options on server socket.");
    }

    if (socket_bind(server, type, host, port) == false) {
        sys::log::NetworkEngine::fatal("Unable to bind server socket. Aborting.");
        socket_close(server);
        return INVALID_SOCKET;
    }

    // Put socket in listen mode with max of SocketServerOptionListenQueueLength new connections in queue.
    if (socket_mode_listen(server, SocketServerOptionListenQueueLength) == false) {
        sys::log::NetworkEngine::fatal("Unable to listen on server socket. Aborting.");
        socket_close(server);
        return INVALID_SOCKET;
    }

    sys::log::NetworkEngine::debug("server socket (%i): ip = %s port = %lu", server, host, port);
    return server;
}


/***
 * Updates internal data for the new connection.
 */
void NetworkEngine::AddNewConnection(SOCKET s) {
    assert(s != INVALID_SOCKET);

    // Since nextCID can never decrease we can safely do this check before acquiring the lock.
    if (nextCID >= MaxConnectionID) {
        sys::log::NetworkEngine::error("Cannot assign a CID, somehow we assigned %i CIDs.", MaxConnectionID);
        sys::log::NetworkEngine::debug("socket (%i): connected - (cid = ?) FAILED (max CID reached)", s);
        socket_close(s);
        return;
    }

    // Initialize the SocketData for the new connection.
// WARNING: The commented line below this has a subtle bug that ISN'T caught by the compiler, since both
//          SOCKET and ConnectionID are typedefs of int's (even if one is unsigned and one is signed).
//    SocketData* sd = SocketData::construct(s, nextCID++);
    SocketData* sd = SocketData::construct(nextCID++, s);
    NetworkMessage* m = NetworkMessage::construct(sd->cid, net::MessageTypes::NewConnection, 0, NULL);
    GameEngine::instance().AddMessageRecv(m);

    uqueue_new.lock();
    uqueue_new.push(sd);

    // Update connection and statistics tracking values.
    users_total++;
    users_current++;
    users_peak = std::max(users_peak, users_current);
    uqueue_new.unlock();

    sys::log::NetworkEngine::debug("socket (%i): connected (cid = %u)", s, sd->cid);
    LogStatus();
}


/***
 * Updates data for the closed connection.
 */
void NetworkEngine::DisconnectConnection(SocketData* sd) {
    assert(sd != NULL);
    assert(sd->s != INVALID_SOCKET);
    assert(sd->cid != InvalidConnectionID);

    sys::log::NetworkEngine::add("socket (%i): disconnected (cid = %u, RX = %lu KiB, TX = %lu KiB)",
            sd->s, sd->cid, sd->rx/1024, sd->tx/1024);

    NetworkMessage* m = NetworkMessage::construct(sd->cid, net::MessageTypes::Disconnection, 0, NULL);
    GameEngine::instance().AddMessageRecv(m);

    NetworkEngine::socket_close(sd->s);
    SocketData::destruct(sd);
    users_current--;
}


// FIXME: Add a version that takes a list and push()es all the elements from that list.
void NetworkEngine::QueueSendMessage(NetworkMessage* m) {
    messagesToSend.lock();
    messagesToSend.push(m);
    messagesToSend.unlock();
}


void NetworkEngine::LogStatus(void) {
    sys::log::NetworkEngine::add("           (users = %5i, peak = %5i, total = %5i)", users_current, users_peak, users_total);
    sys::log::NetworkEngine::add(" uqueue_new.size()    = %i", uqueue_new.size());
    sys::log::NetworkEngine::add(" uqueue_remove.size() = %i", uqueue_remove.size());
    sys::log::NetworkEngine::add(" RX = %lu KiB, TX = %lu KiB", rx_bytes/1024, tx_bytes/1024);
    sys::log::NetworkEngine::add(" threads: accept %u, recv %u, send %u", threads_accept, threads_recv, threads_send);
}


/***
 * Sends the given data to the socket s. If successful the number of bytes
 * sent is returned, else 0 if a temporary error and < 0 if a fatal error
 * occured.
 */
long NetworkEngine::socket_send(SOCKET s, const char *data, std::size_t length) {
    assert(s != INVALID_SOCKET);
    assert(data != NULL);
    assert(length > 0);

    ssize_t result = send(s, data, length, 0);
    // If result is larger than 0 then the write was successful.
    //   We will also include the case where send()/write() returned 0, which
    // is only supposed to happen if the length argument is 0 which would cause
    // an assertion failure for us so we'd never even get here if that was the
    // case.
    if (result >= 0) {
        NetworkEngine::tx_bytes += result;
        ++NetworkEngine::nsocket_send;
        return result;
    }

    // result < 0 that means that an error was encountered. Check if we can
    // handle it...
    int errorValue = get_error_code();

    // NOTE: Write to socket blocked, so a transient error. Just try again next time.
    #ifdef WSAEWOULDBLOCK   // Windows
    if (errorValue == WSAEWOULDBLOCK) return 0;
    #endif
    #ifdef EAGAIN           // POSIX.1-2001 / UNIX
    if (errorValue == EAGAIN) return 0;
    #endif
    #ifdef EWOULDBLOCK      // POSIX.1-2001 / BSD
    if (errorValue == EWOULDBLOCK) return 0;
    #endif
    #ifdef EINTR            // POSIX
    if (errorValue == EINTR) return 0;
    #endif

    // Fatal error. Log it and report so the socket/user can get disconnected.
    sys::log::NetworkEngine::debug("socket (%i): error on write (%i:%s)", s, errorValue, get_error_msg(errorValue));
    return -1;
}


/***
 * Reads from a socket, s, to a specified buffer. If successful the number of
 * bytes read is returned, if a temporary error occurred 0 is returned and if
 * a fatal error occurs a number < 0 is returned.
 */
long NetworkEngine::socket_read(SOCKET s, char *data, std::size_t length) {
    assert(s != INVALID_SOCKET);
    assert(data != NULL);
    assert(length != 0);

    ssize_t result = recv(s, data, length, 0);

    // If received is larger than 0 then the read was successful.
    if (result > 0) {
        assert(result <= static_cast<ssize_t>(length));
        NetworkEngine::rx_bytes += result;
        ++NetworkEngine::nsocket_recv;
        return result;
    }

    // EOF/ordered disconnect, handles the same way as an error but with a slightly different log.
    if (result == 0) {
        sys::log::NetworkEngine::debug("socket (%i): EOF (connection broken by peer)", s);
        return -1;
    }

    // recv() returned a value < 0. There was an error. Check if we know how to handle it...
    int errorValue = get_error_code();

    // NOTE: Read from socket blocked, so a transient error. Just try again next time.
    #ifdef WSAEWOULDBLOCK   // Windows
    if (errorValue == WSAEWOULDBLOCK) return 0;
    #endif
    #ifdef EAGAIN           // POSIX.1-2001 / UNIX
    if (errorValue == EAGAIN) return 0;
    #endif
    #ifdef EWOULDBLOCK      // POSIX.1-2001 / BSD
    if (errorValue == EWOULDBLOCK) return 0;
    #endif
    #ifdef EINTR            // POSIX
    if (errorValue == EINTR) return 0;
    #endif

    // Fatal error. Log it and report so the socket/user can get disconnected.
    sys::log::NetworkEngine::debug("socket (%i): error on read (%i:%s", s, errorValue, get_error_msg(errorValue));
    return -1;
}


/***
 * Closes the given socket.
 */
void NetworkEngine::socket_close(SOCKET s) {
    assert(s != INVALID_SOCKET);

    // FIXME: Detect, log and report errors.

    sys::log::NetworkEngine::debug("socket (%i): close", s);
    #if (PLATFORM == PLATFORM_WINDOWS)
        closesocket(static_cast<int>(s);
    #elif (PLATFORM == PLATFORM_UNIX)
        ::close(static_cast<int>(s));
    #endif
}


// Wrapper for creating a socket, logging it and detecting/logging errors.
SOCKET NetworkEngine::socket_create(int type) {
    assert((type == AF_INET) || (type == AF_INET6));

    SOCKET s = socket(type, SOCK_STREAM, 0);
    if (s != INVALID_SOCKET) {
        sys::log::NetworkEngine::debug("socket (%i): created", s);
    } else {
        sys::log::NetworkEngine::error("socket (?): created - FAILED (%i:%s)", get_error_code(), get_error_msg());
    }
   return s;
}


// Wrapper for binding a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_bind(SOCKET s, int ai_family, const char *bindaddr, IPPort port) {
    assert(s != INVALID_SOCKET);
    assert(((ai_family == AF_INET) || (ai_family == AF_INET6)));

    struct addrinfo ai_hints, *res0;
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = ai_family;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;
    if (bindaddr == NULL)
        ai_hints.ai_flags = AI_PASSIVE;
    ai_hints.ai_canonname = NULL;
    ai_hints.ai_addr = NULL;
    ai_hints.ai_next = NULL;

    char bindport[8];
    snprintf(bindport, 8, "%hu", port);

    if (ai_family == AF_INET6) {
        sys::log::NetworkEngine::debug("socket_bind(): Limiting IPv6 sockets to only listen to IPv6 traffic.");
        if (socket_mode_ipv6only(s) == false) {
            sys::log::NetworkEngine::warning("Failed to set IPv6 socket to only accept IPv6 connections. May cause errors.");
        }
    }

    // FIXME: Walk the list to find the most appropriate address to bind to. But how do we know which that is?
    //        If we request a simple translation from a string representing a numeric IP address can we even
    //        get more than one result?
    int result = 0;

    sys::log::NetworkEngine::debug("getaddrinfo(): addr = %s, port = %s, hints = ...", bindaddr, bindport);
    if ((result = getaddrinfo(bindaddr, bindport, &ai_hints, &res0)) != 0) {

        sys::log::NetworkEngine::warning("socket (%i): bind (ip = %s, port = %s) - FAILED (getaddrinfo -> %i:%s)", bindaddr, bindport, result, gai_strerror(result));

        if (use_strict_bind)
            return false;

        sys::log::NetworkEngine::debug("getaddrinfo(): addr = <all>, port = %s, hints = ...", bindport);
        if ((result = getaddrinfo(NULL, bindport, &ai_hints, &res0)) != 0) {
            sys::log::NetworkEngine::warning("socket (%i): bind (ip = %s, port = %s) - FAILED (getaddrinfo -> %i:%s)", bindaddr, bindport, result, gai_strerror(result));
            return false;
        }

    }

    result = bind(s, res0->ai_addr, res0->ai_addrlen);
    freeaddrinfo(res0);

    if (result == 0) {
        sys::log::NetworkEngine::debug("socket (%i): bind (ip = %s port = %hu)", s, bindaddr, port);
        return true;
    }
    sys::log::NetworkEngine::debug("socket (%i): bind (ip = %s port = %hu) - FAILED (%i:%s)", s, bindaddr, port, get_error_code(), get_error_msg());
    return false;
}


// Wrapper for setting non-blocking mode for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_nonblocking(SOCKET s) {
    assert(s != INVALID_SOCKET);

    #if (PLATFORM == PLATFORM_WINDOWS)
        unsigned long option = 1;
        int e = ioctlsocket(s, FIONBIO, &option);
    #elif (PLATFORM == PLATFORM_UNIX)
        long flags = fcntl(s, F_GETFL, 0);
        if (flags != -1) {
            flags |= O_NONBLOCK;
        } else {
            flags = O_NONBLOCK;
        }
        int e = fcntl(s, F_SETFL, flags);
    #else
        #error "PLATFORM-branch missing implementation."
    #endif

    if (e == -1) {
        sys::log::NetworkEngine::debug("socket (%i): non-blocking mode - FAILED  (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): non-blocking mode", s);
    return true;
}


// Wrapper for setting listening mode for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_listen(SOCKET s, int n) {
    assert(s != INVALID_SOCKET);

    if (listen(s, n) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): listen mode (%i queued) - FAILED (%i:%s)", s, n, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): listen mode (%i queued)", s, n);
    return true;
}


// Wrapper for setting option REUSEADDR for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_reuseaddr(SOCKET s) {
    assert(s != INVALID_SOCKET);

    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, static_cast<const void*>(&on), sizeof(on)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_REUSEADDR) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_REUSEADDR)", s);
    return true;
}


// Wrapper for setting option LINGER for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_linger(SOCKET s) {
    assert(s != INVALID_SOCKET);

    struct linger ld = {0, 0};
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, static_cast<const void*>(&ld), sizeof(ld)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_LINGER) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_LINGER)", s);
    return true;
}


// Wrapper for setting option IPV6_V6ONLY for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_ipv6only(SOCKET s) {
    assert(s != INVALID_SOCKET);

    int on = 1;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, static_cast<const void*>(&on), sizeof(on)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (IPV6_V6ONLY) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (IPV6_V6ONLY)", s);
    return true;
}


// Wrapper for setting option KEEPALIVE for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_keepalive(SOCKET s) {
    assert(s != INVALID_SOCKET);

    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, static_cast<const void*>(&on), sizeof(on)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_KEEPALIVE) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_KEEPALIVE)", s);
    return true;
}


// Wrapper for setting option TIMESTAMP for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_timestamp(SOCKET s) {
    assert(s != INVALID_SOCKET);

    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP, static_cast<const void*>(&on), sizeof(on)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_TIMESTAMP) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (SO_TIMESTAMP)", s);
    return true;
}


// Wrapper for setting option NODELAY for a socket, logging it and detecting/logging errors.
bool NetworkEngine::socket_mode_nodelay(SOCKET s) {
    assert(s != INVALID_SOCKET);

    int on = 1;
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, static_cast<const void*>(&on), sizeof(on)) != 0) {
        sys::log::NetworkEngine::debug("socket (%i): setsockopt (TCP_NODELAY) - FAILED (%i:%s)", s, get_error_code(), get_error_msg());
        return false;
    }
    sys::log::NetworkEngine::debug("socket (%i): setsockopt (TCP_NODELAY)", s);
    return true;
}


/***
 * Returns the most recent socket error code.
 */
inline int NetworkEngine::get_error_code(void) {

    #if (PLATFORM == PLATFORM_WINDOWS)
        return WSAGetLastError();
    #elif (PLATFORM == PLATFORM_UNIX)
        return errno;
    #else
        #error "PLATFORM-branch missing implementation."
    #endif
}


/***
 * Returns a string for the most recent socket error code.
 */
inline const char* NetworkEngine::get_error_msg(int e) {
    if (e == 0)
        e = get_error_code();

    #if (PLATFORM == PLATFORM_WINDOWS)
//        return WSAGetLastError();
        return strerror(e);
    #elif (PLATFORM == PLATFORM_UNIX)
        return strerror(e);
    #else
        #error "PLATFORM-branch missing implementation."
    #endif
}


#ifdef NETWORKENGINE_DEPRECATED
/***
 * Translates a given string into an IP address if possible.
 */
IPv4 NetworkEngine::resolveAddress(const char *address) {
    assert(address != NULL);

    // Declare some data and try to translate the argument to an IP-address.
    struct hostent *host;
        IPv4 IPnumber = inet_addr(address);

    // If no valid IP-address then assume the address to be a domain-name and
    // try to translate that instead.
    if (IPnumber != INADDR_NONE)
        return IPnumber;

    host = gethostbyname((const char*) address);
    // Was the lookup successful? If we didn't get NULL then it was, else
    // the host wasn't found or an error occurred.
    if (host != NULL)
        return *((unsigned long*) host->h_addr_list[0]);

    return 0;
}


/***
 * Resolves the host a socket is connected to. Memory will be allocated for the
 * string so it has to be deallocated by the receiver when not used anymore.
 */
char* NetworkEngine::GetHostOfSocket(SOCKET s) {
    assert(s != INVALID_SOCKET);

    struct sockaddr_in lPeer;
    socklen_t lLength = sizeof(lPeer);
    char *lAddr = NULL, *lTemp = NULL;
    struct hostent *lHost = NULL;

    if (getpeername(s, (struct sockaddr*) &lPeer, &lLength) == 0) {
        if (useNameserver)
            lHost = gethostbyaddr((const char*) &lPeer.sin_addr, sizeof(lPeer.sin_addr), AF_INET);

        if (lHost != NULL) {
            lAddr = new char[strlen(lHost->h_name) + 1];
            strcpy(lAddr, lHost->h_name);
        } else {
            lTemp = inet_ntoa(lPeer.sin_addr);
            if (lTemp != NULL) {
                lAddr = new char[strlen(lTemp) + 1];
                strcpy(lAddr, lTemp);
            } else {
                xlog.debug("SYSERR: Could not get host nor address of socket %i.", s);
            }
        }
    }

    xlog.debug("socket (%i): hostname \"%s\"", (int)s, lAddr);
    return lAddr;
}


void NetworkEngine::lookupEndpoint(ConnectionData *connection) {
    assert(connection != NULL);
    assert(connection->uSocket != INVALID_SOCKET);

    struct sockaddr_in lPeer;
    socklen_t lLength = sizeof(lPeer);
    struct hostent *lHost;

    if (getpeername(connection->uSocket, (struct sockaddr*) &lPeer, &lLength) != 0)
        return;

    connection->IP = lPeer.sin_addr.s_addr;
    if (useNameserver == false)
        return;

    lHost = gethostbyaddr((const char*) &(lPeer.sin_addr), sizeof(lPeer.sin_addr), AF_INET);
    if (lHost != NULL) {
        strncpy(connection->hostname, lHost->h_name, SIZE_MaxBufferSize-1);
    }
}
#endif // NETWORKENGINE_DEPRECATED





#ifdef TEMP_SOCKET_DEBUG_FUNCTIONS
// TODO: Rework the following functions into a "proper" set of socket debug functions?
//       It could be quite handy to have some function to automatically find and print all pertinent info
//       regarding a socket (in whichever state it is in) and the same for the content of all those address
//       structures.

const char* sockaddr_to_string(struct sockaddr_in *addr) {
    static char ipv4[64];
    char buf[32];

    ipv4[0] = '\0';
    for (int i = 0; i < 4; i++) {
        unsigned char c = reinterpret_cast<unsigned char*>(&(addr->sin_addr))[i];
        int n = static_cast<int>(c);
        sprintf(buf, "%i.", n);
        strcat(ipv4, buf);
    }
    ipv4[strlen(ipv4)-1] = '\0';

    return ipv4;
}
const char* sockaddr_to_string(struct sockaddr_in6 *addr) {
    static char ipv6[64];
    char buf[32];

    ipv6[0] = '\0';
    for (int i=0; i < 8; i++) {
        sprintf(buf, "%hx:", reinterpret_cast<short int *>(&(addr->sin6_addr))[i]);
        strcat(ipv6, buf);
    }
    ipv6[strlen(ipv6)-1] = '\0';

    return ipv6;
}

void print_socket_stats(SOCKET s) {

    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    printf("pss: 10\n"); fflush(stdout);
    if (getsockname(s, reinterpret_cast<struct sockaddr *>(&addr), &addrlen) == 0) {
        printf("pss: 11\n"); fflush(stdout);
        if (addr.ss_family == AF_INET) {
            printf("pss: 12\n"); fflush(stdout);
            int p = static_cast<int>(reinterpret_cast<struct sockaddr_in*>(&addr)->sin_port);
            printf("*** %i: ipv4 = %s port = %i\n", s, sockaddr_to_string(reinterpret_cast<struct sockaddr_in*>(&addr)), p);
        }
        if (addr.ss_family == AF_INET6) {
            printf("pss: 13\n"); fflush(stdout);
            int p = static_cast<int>(reinterpret_cast<struct sockaddr_in6*>(&addr)->sin6_port);
            printf("*** %i: ipv6 = %s port = %i\n", s, sockaddr_to_string(reinterpret_cast<struct sockaddr_in6*>(&addr)), p);
        }
    }
    printf("pss: 20\n"); fflush(stdout);

    int optval;
    socklen_t optlen = sizeof(optval);

    if (getsockopt(s, SOL_SOCKET, SO_PROTOCOL, &optval, &optlen) == 0) {
        printf("%i: SO_PROTOCOL = %i\n", s, optval);
//    IPPROTO_SCTP
    }
    if (getsockopt(s, SOL_SOCKET, SO_TYPE, &optval, &optlen) == 0) {
        printf("%i: SO_TYPE = %i\n", s, optval);
//    SOCK_STREAM
    }
}
#endif // TEMP_SOCKET_DEBUG_FUNCTIONS

} // namespace net
