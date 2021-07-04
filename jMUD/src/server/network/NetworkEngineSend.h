#ifndef NETWORKENGINESEND_H
#define NETWORKENGINESEND_H

#include "NetworkEngineThread.h"   //
#include "UnorderedArray.h" // UnorderedArray
#include "NetworkCore.h"


namespace net {


class NetworkEngineSend : public NetworkEngineThread {
public:
    NetworkEngineSend(const char * n);
    ~NetworkEngineSend();

    bool run(void);

private:
    NetworkEngineSend(const NetworkEngineSend&);
    NetworkEngineSend& operator=(const NetworkEngineSend&);

    void exec(void);
};


} // namespace net

#endif // NETWORKENGINESEND_H
