#ifndef NETWORKENGINERECV_H
#define NETWORKENGINERECV_H

#include "NetworkEngineThread.h"   //
#include "NetworkCore.h"

#include <mutex>            // std::mutex
#include <vector>           // std::vector


namespace net {


class NetworkEngineRecv : public NetworkEngineThread {
public:
    NetworkEngineRecv(const char* n);
    ~NetworkEngineRecv();

    bool run(void);

private:
    NetworkEngineRecv(const NetworkEngineRecv&);
    NetworkEngineRecv& operator=(const NetworkEngineRecv&);

    void exec(void);
    void fetch_new_connections(void);
    bool read_data(SocketData* sd);

    void purge_select_set(void);

    std::mutex mutex_data;
    std::vector<SocketData*> sockets;

    #if (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
        fd_set fdset;
        SOCKET socket_max;
    #elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
        int epoll_fd;
        struct epoll_event event;
        struct epoll_event *events;
    #endif
};


} // namespace net

#endif // NETWORKENGINERECV_H
