
#include "../../config/config.h"
#include "NetworkEngineRecv.h"
#include "NetworkEngine.h"
#include "NetworkCore.h"
#include "../GameEngine.h"

#include <mutex>                // std::mutex
#include <algorithm>            // find(...)
#include <sys/types.h>          // *for compability*
#include <sys/socket.h>         // getsockopt()
#include <functional>


namespace net {


NetworkEngineRecv::NetworkEngineRecv(const char *n) :
    mutex_data(),
    sockets(),
    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        socket_max(-1)
    #elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
        epoll_fd(-1),
        event(),
        events(NULL)
    #endif
{
    // Allocate memory and copy the name of the thread.
    if (n == NULL) {
        sys::log::NetworkEngine::warning("NetworkEngineRecv will be unnamed.");
        name = NULL;
    } else {
        name = new char[strlen(n)+1];
        if (name == NULL) {
            sys::log::NetworkEngine::warning("Failed to allocate memory for NetworkEngineRecv::name. NetworkEngineRecv will be unnamed.");
            name = NULL;
        }
        strcpy(name, n);
    }

    #if (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
        epoll_fd = epoll_create1 (0);
        if (epoll_fd == -1) {
            sys::log::NetworkEngine::error("<%s> Failed to create an epoll file descriptor. Aborting. (%i:%s)", NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
            return;
        }

        /* Buffer where events are returned */
        events = new epoll_event[NetworkEngine::MaxSocketsPerThread];
        if (events == NULL) {
            sys::log::NetworkEngine::error("<%s> Failed to allocate events structure for epoll. Aborting. (%i:%s)");
            return;
        }
    #endif

    sockets.reserve(NetworkEngine::MaxSocketsPerThread);
    initialized = true;

    sys::log::NetworkEngine::debug("NetworkEngineRecv <%s> created", name);
}


NetworkEngineRecv::~NetworkEngineRecv() {
    for (std::size_t i = 0; i < sockets.size(); i++) {
        NetworkEngine::instance().DisconnectConnection(sockets.back());
        sockets.pop_back();
    }

    delete[] name;
    delete t;
}


bool NetworkEngineRecv::run(void) {
    std::function<void (NetworkEngineRecv*)> f;
    f = &NetworkEngineRecv::exec;

    if (initialized == false) {
        sys::log::NetworkEngine::error("<%s> Is not properly initialized. Not running.", name);
        return false;
    }

    if (t != NULL) {
        sys::log::NetworkEngine::warning("<%s> Thread is already running.", name);
        return false;
    }

    t = new std::thread(f, this);
    if (t == NULL) {
        sys::log::NetworkEngine::error("<%s> failed to create a thread to run in.", name);
        return false;
    }

    return true;
}


void NetworkEngineRecv::exec(void) {
    assert(t != NULL);
    assert(initialized == true);
    assert(running == false);

    sys::log::NetworkEngine::add("<%s> Starting...", name);
//    struct timespec req = { 0, 500 * 1000*1000};   // 1000 ms
    struct timespec req = { 1, 0};   // 1000 ms
    struct timespec res = { 0, 0};
    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        struct timeval  tv  = { 0, 500 * 1000};   // 500 ms
    #endif

    running = true;
    while (!NetworkEngine::instance().shutdown()) {

        if (!NetworkEngine::instance().uqueue_new.empty() && (sockets.size() < NetworkEngine::MaxSocketsPerThread)) {
            sys::log::NetworkEngine::debug("<%s> fetching new connections...", name);
            fetch_new_connections();
        }

        if (!sockets.empty()) {
            #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)

                fd_set recvset;
                memcpy(&recvset, &fdset, sizeof(fd_set));

                tv.tv_sec = 0; tv.tv_usec = 500 * 1000;   // 500 ms
//                sys::log::NetworkEngine::debug("<%s> %lu connection(s) - blocking on select (max %lis %lims %lius)", name, sockets.size(), tv.tv_sec, tv.tv_usec / 1000, tv.tv_usec % 1000);
                int readySockets = select(socket_max, &recvset, NULL, NULL, &tv);
                if (readySockets == -1) {
                    if (NetworkEngine::get_error_code() == EBADF) {
                        sys::log::NetworkEngine::add("<%s> select() reported bad file descriptors. Purging bad ones.", name);
                        purge_select_set();
                        continue;
                    } else if (NetworkEngine::get_error_code() == EINTR) {
                        continue;
                    } else {
                        sys::log::NetworkEngine::debug("<%s> select() - FAILED (%i:%s)", name, NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
                        break;
                    }
                }

                if (readySockets > 0 ) {
                    sys::log::NetworkEngine::debug("<%s> %lu connection(s) - processing input from %i connection(s)", name, sockets.size(), readySockets);
                    mutex_data.lock();

                    std::vector<SocketData*>::iterator it = sockets.begin();
                    while (it != sockets.end()) {
                        if (FD_ISSET((*it)->s, &recvset)) {
//                            sys::log::NetworkEngine::verbose("<%s> socket (%i): has input available", name, (*it)->s);
                            if (read_data(*it) == false) {
                                FD_CLR((*it)->s, &fdset);  // Remove from our fd_set.
                                NetworkEngine::instance().DisconnectConnection(*it);   //
                                it = sockets.erase(it); // Erase from the connection list.
                            }
                        }
//                        if (it == sockets.end())
//                            break;
                        ++it;
                    }
                    mutex_data.unlock();
                } else {
//                    sys::log::NetworkEngine::debug("<%s> %lu connection(s) - zero input", name, sockets.size());
                }

            #elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)

                // FIXME: Remove that "magic" value for the timeout time.
                int eventCount = epoll_wait(epoll_fd, events, NetworkEngine::MaxSocketsPerThread, 500);
                if (eventCount == -1) {
                    if (NetworkEngine::get_error_code() == EINTR)
                        continue;
                    sys::log::NetworkEngine::debug("<%s> epoll_wait() - FAILED (%i:%s)", name, NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
                    break;
                }

                if (eventCount > 0 ) {
                    sys::log::NetworkEngine::debug("<%s> %lu connection(s) - processing events from %i connection(s).", name, sockets.size(), eventCount);
                    mutex_data.lock();

                    for (unsigned int i = 0; i < static_cast<unsigned int>(eventCount); i++) {

                        // TODO: Couldn't this be done in a more elegant way? Since we can get any data
                        //       we want back couldn't we avoid doing any search...
                        //       Of course it is possible, just use the data directly and only search
                        //       for its location in the "list" if it needs to be removed.
                        std::vector<SocketData*>::iterator it;
                        it = find(sockets.begin(), sockets.end(), static_cast<SocketData*>(events[i].data.ptr));
                        if (it == sockets.end()) {
                            sys::log::NetworkEngine::error("<%s> socket (??): returned from epoll_wait() but not found in connection list.", name);
                            continue;
                        }

                        if ((events[i].events & EPOLLIN) || (events[i].events & EPOLLPRI)) {
                            sys::log::NetworkEngine::verbose("<%s> socket (%i): has input available", name, (*it)->s);
                            if (read_data(*it) == true)
                                continue;
                        }

                        // If we get here no input was available, so some error occured and we will close
                        // and remove the socket.
                        sys::log::NetworkEngine::verbose("<%s> socket (%i): removing", name, (*it)->s);
                        if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (events[i].events & EPOLLRDHUP)) {
                            sys::log::NetworkEngine::error("<%s> socket (%i): ERROR (%i:%s)", name, (*it)->s, NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
                        }
                        epoll_ctl (epoll_fd, EPOLL_CTL_DEL, (*it)->s, NULL);
                        NetworkEngine::instance().DisconnectConnection(*it);   //
                        it = sockets.erase(it);
                    }

                    mutex_data.unlock();
                } else {
//                    sys::log::NetworkEngine::debug("<%s> %lu connection(s) - zero input", name, sockets.size());
                }

            #endif // (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)

        } else {
            sys::log::NetworkEngine::verbose("<%s> 0 connections (sleeping for max %lis %lims %lius %lins)", name, req.tv_sec, req.tv_nsec / 1000000, (req.tv_nsec % 1000000) / 1000, req.tv_nsec % 1000);
            // In the case we got woken up early because of a signal we just continue as if nothing
            // happened since we don't really care exactly how long we slept, just that we do sleep some.
            nanosleep(&req, &res);
        }
    }

    if (!sockets.empty()) {
        std::size_t size_old = sockets.size();
        sys::log::NetworkEngine::debug("<%s> %lu connection(s) - autoclosing...", name, sockets.size());
        for (size_t i = 0; i < sockets.size(); i++) {
            sys::log::NetworkEngine::verbose("<%s>   socket (%i): autoclosed", name, sockets.at(i)->s);
            #if (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
                epoll_ctl (epoll_fd, EPOLL_CTL_DEL, sockets.at(i)->s, NULL);
            #endif
            NetworkEngine::instance().DisconnectConnection(sockets.at(i));   //
        }
        sys::log::NetworkEngine::debug("<%s> %lu connection(s) - autoclosed", name, size_old - sockets.size());
    }

    sys::log::NetworkEngine::add("<%s> Terminating.", name);
}


void NetworkEngineRecv::fetch_new_connections(void) {
    NetworkEngine::instance().uqueue_new.lock();

    std::size_t size_old = sockets.size();
    while (!NetworkEngine::instance().uqueue_new.empty() && (sockets.size() < NetworkEngine::MaxSocketsPerThread)) {
        SocketData* sd = NetworkEngine::instance().uqueue_new.pop();

        sys::log::NetworkEngine::debug("<%s> adding socket = %i with cid = %u", name, sd->s, sd->cid);

        #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wold-style-cast"
            FD_SET(sd->s, &fdset);
            #pragma GCC diagnostic pop
            socket_max = std::max(socket_max, sd->s + 1);
        #elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
//            event.data.fd = sd->s;
// TODO: Pass a pointer to the appropriate SocketData for each socket watched.
            event.data.ptr = sd;
            event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
            if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, sd->s, &event) == -1) {
                sys::log::NetworkEngine::error("<%s> epoll_ctl(): adding socket (%i) - FAILED (%i:%s)", name, sd->s, NetworkEngine::get_error_code(), NetworkEngine::get_error_msg());
                NetworkEngine::instance().DisconnectConnection(sd);
                continue;
            }
        #endif

        sys::log::NetworkEngine::verbose("<%s>   socket (%i): transfered", name, sd->s);
        sockets.push_back(sd);
    }
    NetworkEngine::instance().uqueue_new.unlock();

    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        sys::log::NetworkEngine::verbose("<%s>   socket_max = %i", name, socket_max);
    #endif
    sys::log::NetworkEngine::debug("<%s> %lu connection(s) transfered @ %i/%i connections", name, sockets.size() - size_old, sockets.size(), NetworkEngine::MaxSocketsPerThread);

    if (NetworkEngine::instance().users_current > (NetworkEngine::instance().threads_recv * NetworkEngine::SocketsPerThreadHigh)) {
        char n[16];
        snprintf(n, 15, "Recv%u:", NetworkEngine::instance().threads_recv + 1);
        NetworkEngine::instance().SpawnRecvThread(n);
        NetworkEngine::instance().LogStatus();
    }
}


bool NetworkEngineRecv::read_data(SocketData* sd) {
    assert(sd != NULL);
    assert(sd->s != INVALID_SOCKET);
    assert(sd->cid != InvalidConnectionID);

    char a[1024*64];
    NetworkMessage* m = NULL;

    long int length = NetworkEngine::socket_read(sd->s, a, 1024*64);
    if (length > 0) {
        sys::log::NetworkEngine::verbose("<%s> socket (%i): read %li bytes", name,  sd->s, length);
        sd->rx += length;
        char* tmpBuffer = new char[SIZE_MaxBufferSize];
        memcpy(tmpBuffer, a, length);
        m = NetworkMessage::construct(sd->cid, net::MessageTypes::DataIncoming, length, tmpBuffer);
        GameEngine::instance().AddMessageRecv(m);
    } else if (length < 0) {
        sys::log::NetworkEngine::verbose("<%s> socket (%i): read FAILED (disconnecting)", name, sd->s);
        return false;
    }

    return true;
}


void NetworkEngineRecv::purge_select_set(void) {
    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)

    sys::log::NetworkEngine::debug("<%s> purge_select_set() - rebuilding fd_set...", name);

    FD_ZERO(&fdset);
    std::vector<SocketData*>::iterator it = sockets.begin();
    while (it != sockets.end()) {
        FD_SET((*it)->s, &fdset);
        ++it;
    }

    #endif
}

} // namespace net
