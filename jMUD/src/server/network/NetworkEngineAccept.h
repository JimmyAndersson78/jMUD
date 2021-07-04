#ifndef NETWORKENGINEACCEPT_H
#define NETWORKENGINEACCEPT_H

#include "NetworkEngineThread.h"   //
#include "UnorderedArray.h" // UnorderedArray
#include "NetworkCore.h"

#include <mutex>            // std::mutex


namespace net {


class NetworkEngineAccept : public NetworkEngineThread {
public:
    NetworkEngineAccept(const char* n, const char* addr, IPPort port, int type);
    ~NetworkEngineAccept();

    bool run(void);

private:
    NetworkEngineAccept(const NetworkEngineAccept&);
    NetworkEngineAccept& operator=(const NetworkEngineAccept&);

    void exec(void);

    SOCKET server;
};


} // namespace net

#endif // NETWORKENGINEACCEPT_H
