#include <functional>

#include "config.h"
#include "NetworkEngineAccept.h"
#include "NetworkEngine.h"
#include "NetworkCore.h"

#if (PLATFORM == PLATFORM_UNIX)
    #include <arpa/inet.h>      // inet_addr()
    #include <netdb.h>          // NI_MAXHOST
#endif


namespace net {


NetworkEngineAccept::NetworkEngineAccept(const char* n, const char* addr, IPPort port, int type) :
    server(INVALID_SOCKET)
{
    // Allocate memory and copy the name of the thread.
    if (n == NULL) {
        sys::log::NetworkEngine::warning("NetworkEngineAccept will be unnamed.");
        name = NULL;
    } else {
        name = new char[strlen(n)+1];
        if (name == NULL) {
            sys::log::NetworkEngine::warning("Failed to allocate memory for SocketThread::name. NetworkEngineAccept will be unnamed.");
            name = NULL;
        }
        strcpy(name, n);
    }

    // Verify that the given address family is one we can handle.
    switch (type) {
    case AF_INET:
    case AF_INET6:
        break;
    default:
        sys::log::NetworkEngine::error("<%s> Unknown address family requested. (af = , ip = %s, port = %lu)", type, addr, port);
        return;
    }

    if (port == 0) {
        sys::log::NetworkEngine::warning("<%s> Will bind to a random port.");
    }

    // Setup a server listening socket.
    server = NetworkEngine::setup_server_socket(type, addr, port);
    if (server == INVALID_SOCKET) {
        sys::log::NetworkEngine::error("<%s> Failed to setup server socket.", name);
        return;
    }
    sys::log::NetworkEngine::debug("socket (%i): server socket for '%s'", server, name);
    sys::log::add("Accepting connections @ ip = %s port = %lu", addr, port);

    initialized = true;

    sys::log::NetworkEngine::debug("NetworkEngineAccept <%s> created", name);
}


NetworkEngineAccept::~NetworkEngineAccept() {
    NetworkEngine::socket_close(server);
    NetworkEngine::socket_close(control);
    delete[] name;
    delete t;
}


bool NetworkEngineAccept::run(void) {
    std::function<void (NetworkEngineAccept*)> f;
    f = &NetworkEngineAccept::exec;

    if (initialized == false) {
        sys::log::NetworkEngine::error("<%s> Is not properly initialized. Not running.", name);
        return false;
    }

    if (t != NULL) {
        sys::log::NetworkEngine::warning("<%s> Thread is already running.", name);
        return false;
    }

    // We are now ready to spawn a thread and run in it.
    t = new std::thread(f, this);
    if (t == NULL) {
        sys::log::NetworkEngine::error("<%s> failed to create a thread to run in.", name);
        return false;
    }

    return true;
}


void NetworkEngineAccept::exec(void) {
    sys::log::NetworkEngine::add("<%s> Starting...", name);

    struct sockaddr_storage addr;
    socklen_t size = sizeof(addr);
    SOCKET s = INVALID_SOCKET;

    while (!NetworkEngine::instance().shutdown()) {
        memset(&addr, 0, size);

        sys::log::NetworkEngine::add("<%s> blocking on accept() for a new connection", name);
        // Accept the new connection.
        s = static_cast<SOCKET>(accept(server, reinterpret_cast<struct sockaddr*>(&addr), &size));

        if (s == INVALID_SOCKET) {
            switch (NetworkEngine::instance().get_error_code()) {
            case EAGAIN:
            #if (EAGAIN != EWOULDBLOCK)
            case EWOULDBLOCK:
            #endif
                //  POSIX.1-2001 - Non-blocking sockets with no pending connections. Trying agin.
                continue;

            case EINTR:
                // NOTE: The system call was interrupted by a signal before finishing. Trying again.
                continue;

            case EMFILE:
            case ENFILE:
                // FIXME: The process can't open more file descriptors, so log the error but keep on going
                //        since the error is semi-transient and in any case not fatal for the already
                //        established connections. We should log this error once every minute or so, and
                //        the first time we should also decrease the maximum number of allowed connections.
                continue;

            case ENETDOWN:
            case EPROTO:
            case ENOPROTOOPT:
            case EHOSTDOWN:
            case ENONET:
            case EHOSTUNREACH:
            case EOPNOTSUPP:
            case ENETUNREACH:
                // NOTE: Possibly leftover errors from a previous connection if the socket was recently
                //       reused. We'll let them pass, but maybe we should keep track of how many of these
                //       we get and after some threshold report them too.
                continue;

            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENOTSOCK:
                // NOTE: One, or more, of the arguments are bad so we'll abort.
                break;

            default:
                break;
            }
            sys::log::NetworkEngine::debug("<%s> Error on accept()'ing connection' (%i:%s)", name, NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
            continue;
        }

        // NOTE: If we are either limited by select()-based polling or max open file descriptors we drop
        //       connections (hopefully) slightly before reaching that limit.
        if (NetworkEngine::instance().GetNumConnections() >= NetworkEngine::instance().GetMaxConnectionsTotal()) {
            sys::log::NetworkEngine::add("<%s> Max connection limit reached. Dropping connection.", name);
            sys::log::NetworkEngine::debug("socket (%i): accepted - FAILED (Connection Limit Reached)", s);
            NetworkEngine::socket_close(s);
            continue;
        }
        #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        if (s >= FD_SETSIZE) {
            sys::log::NetworkEngine::add("<%s> Too many open file descriptors. Dropping connection.", name);
            sys::log::NetworkEngine::debug("socket (%i): accepted - FAILED (Too many open file descriptors)", s);
            NetworkEngine::socket_close(s);
            continue;
        }
        #endif

        sys::log::NetworkEngine::debug("socket (%i): accepted", s);

        // Set appropriate modes for the socket.
        NetworkEngine::socket_mode_nonblocking(s);
        NetworkEngine::socket_mode_linger(s);
        NetworkEngine::socket_mode_keepalive(s);
        NetworkEngine::socket_mode_timestamp(s);

        #ifdef DEBUG
            int size_send = -1, size_recv = -1;
            socklen_t length = sizeof(socklen_t);
            getsockopt(s, SOL_SOCKET, SO_SNDBUF, static_cast<void*>(&size_send), &length);
            getsockopt(s, SOL_SOCKET, SO_RCVBUF, static_cast<void*>(&size_recv), &length);
            sys::log::NetworkEngine::debug("socket (%i): buffers (send=%i KiB, recv=%i KiB)", s, size_send/1024, size_recv/1024);
        #endif // DEBUG

        char peer_name[NI_MAXHOST] = {"<unknown>"}, peer_ip[128], peer_port[8];
        // Make a DNS lookup for the domain name of the connected host.
        if (NetworkEngine::use_dns_lookup) {
            if (getnameinfo (reinterpret_cast<struct sockaddr*>(&addr), size, peer_name, sizeof(peer_name), NULL, 0, 0) != 0) {
                strcpy(peer_name, "<unknown>");
            }
        }
        // Get the IP address of the new connection.
        if (getnameinfo (reinterpret_cast<struct sockaddr*>(&addr), size, peer_ip, sizeof(peer_ip), peer_port, 8, NI_NUMERICHOST) != 0) {
           strcpy(peer_ip, "<unknown>");
        }
        sys::log::NetworkEngine::add("socket (%i): peer = %s (ip = %s port = %s) (server: %s)", s, peer_name, peer_ip, peer_port, name);
        NetworkEngine::instance().AddNewConnection(s); // Register the connection as alive.
    }

    sys::log::NetworkEngine::add("<%s> Terminating.", name);
}

} // namespace net
