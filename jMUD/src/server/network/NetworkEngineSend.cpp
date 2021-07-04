#include <functional>

#include "NetworkEngineSend.h"
#include "NetworkEngine.h"


namespace net {


NetworkEngineSend::NetworkEngineSend(const char* n) {
    // Allocate memory and copy the name of the thread.
    if (n == NULL) {
        sys::log::NetworkEngine::warning("NetworkEngineSend will be unnamed.");
        name = NULL;
    } else {
        name = new char[strlen(n)+1];
        if (name == NULL) {
            sys::log::NetworkEngine::warning("Failed to allocate memory for NetworkEngineSend::name. NetworkEngineSend will be unnamed.");
            name = NULL;
        }
        strcpy(name, n);
    }

    initialized = true;

    sys::log::NetworkEngine::debug("NetworkEngineSend <%s> created", name);
}


NetworkEngineSend::~NetworkEngineSend() {
    delete[] name;
    delete t;
}


bool NetworkEngineSend::run() {
    std::function<void (NetworkEngineSend*)> f;
    f = &NetworkEngineSend::exec;

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


void NetworkEngineSend::exec(void) {
    sys::log::NetworkEngine::add("<%s> Starting...", name);

    // FIXME: Move this time specification into the NetworkEngine class, and make it run-time configurable.
    //        Track uqueue_remove_size_peek, if we ever get close to it this delay should be reduced and
    //        perhaps we should even add some automatic scaling of it... Say if we remove a connection then
    //        the next delay is shorter, but after a few times of nothing to remove we go back to a longer
    //        delay.
    struct timespec req = { 0, 750 * 1000 * 1000}; // 250 milliseconds
    struct timespec rem = { 0, 0};

    while (!NetworkEngine::instance().terminate()) {
        if (!NetworkEngine::instance().uqueue_remove.empty()) {
            sys::log::NetworkEngine::debug("<%s> fetching removed connections...", name);
            NetworkEngine::instance().uqueue_remove.lock();

            std::size_t size_old = NetworkEngine::instance().uqueue_remove.size();
            while (!NetworkEngine::instance().uqueue_remove.empty()) {
                SocketData* sd = NetworkEngine::instance().uqueue_remove.pop();

//                sys::log::NetworkEngine::add("   socket (%i): disconnected (cid = %u, RX = %lu KiB, TX = %lu KiB)", sd->s, sd->cid, sd->rx / 1024, sd->tx / 1024);
                sys::log::NetworkEngine::add("   socket (%i): disconnected (cid = %u, RX = %lu bytes, TX = %lu bytes)",
                           sd->s, sd->cid, sd->rx, sd->tx);
                NetworkEngine::socket_close(sd->s);
                SocketData::destruct(sd);
            }
            NetworkEngine::instance().uqueue_remove.unlock();
            sys::log::NetworkEngine::debug("<%s> %lu connection(s) removed", name, size_old - NetworkEngine::instance().uqueue_remove.size());

        }
        sys::log::NetworkEngine::verbose("<%s> sleeping (max %lis %lims %lius %lins)", name, req.tv_sec, req.tv_nsec / 1000000, (req.tv_nsec % 1000000) / 1000, req.tv_nsec % 1000);
        nanosleep(&req, &rem);
    }

    // TODO: Trigger a logging of all statistics NetworkEngine has, since we are closing down.

    sys::log::NetworkEngine::add("<%s> Terminating.", name);
}

} // namespace net
