#ifndef NETWORKENGINETHREAD_H
#define NETWORKENGINETHREAD_H

#include "config.h"
#include "log.h"
#include "NetworkCore.h"

#include <thread>           // std::thread
#include <cstring>


namespace net {


class NetworkEngineThread {
public:
    NetworkEngineThread():
        t(NULL),
        control(INVALID_SOCKET),
        name(NULL),
        initialized(false),
        running(false) {}
    virtual ~NetworkEngineThread() {}

    virtual bool run(void) = 0;

protected:
    std::thread* t;
    SOCKET control;
    char* name;

    bool initialized;   // Set if the thread is properly initialized.
    bool running;       // Set if the thread is running.

private:
    NetworkEngineThread(const NetworkEngineThread&);
    NetworkEngineThread& operator=(const NetworkEngineThread&);
};


} // namespace net

#endif // NETWORKENGINETHREAD_H
