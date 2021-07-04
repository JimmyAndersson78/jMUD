#ifndef NETWORKCORE_H
#define NETWORKCORE_H

#include "config.h"
#include <chrono>
#include <cassert>



#define NETWORK_POLLING_USE_SELECT      1
#define NETWORK_POLLING_USE_EPOLL       2

//#define NETWORK_POLLING NETWORK_POLLING_USE_SELECT

// Define epoll() or select() usage, unless it has already been done.
#ifndef NETWORK_POLLING
    #if (PLATFORM == PLATFORM_WINDOWS)
        #define NETWORK_POLLING     NETWORK_POLLING_USE_SELECT
    #elif (PLATFORM == PLATFORM_UNIX)
        #if (SYSTEM == SYSTEM_LINUX)
            #define NETWORK_POLLING     NETWORK_POLLING_USE_EPOLL
        #else
            #define NETWORK_POLLING     NETWORK_POLLING_USE_SELECT
        #endif
    #else
        #define NETWORK_POLLING     NETWORK_POLLING_USE_SELECT
    #endif
#endif

// Check that we have a valid combination of polling system and system defined.
#if   (NETWORK_POLLING == NETWORK_POLLING_USE_SELECT)
    // All supported systems support select() based polling.
#elif (NETWORK_POLLING == NETWORK_POLLING_USE_EPOLL)
    #if (PLATFORM == PLATFORM_UNIX) && (SYSTEM == SYSTEM_LINUX)
        #include <sys/epoll.h>
    #else
        #error NetworkEngine: Defined epoll usage for a non-epoll capable system.
    #endif
#else
    #error NetworkEngine: Undefined network polling method defined.
#endif


// WARNING: Windows: FD_SETSIZE has to be defined before including winsock2.h
#ifndef FD_SETSIZE
    #define FD_SETSIZE          1024        // Number of sockets in a set.
#endif


#if (PLATFORM == PLATFORM_WINDOWS)
    #include <winsock2.h>

    typedef unsigned int socklen_t;

    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET      (static_cast<SOCKET>(~0))
    #endif
#elif (PLATFORM == PLATFORM_UNIX)
    #if (SYSTEM != SYSTEM_CYGWIN)
//        #include <stropts.h>
        #include <unistd.h>
    #endif

    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET      (static_cast<SOCKET>(-1))
    #endif
#else
#endif

#include <limits>
#include <stack>    // std::stack<T>
#include <mutex>    // std::mutex


namespace net {


typedef int SOCKET;
typedef uint16_t IPPort; // [0,65535]


#ifndef SOCKET_ERROR
    #define SOCKET_ERROR    (-1)
#endif
#ifndef INADDR_ANY
    #define INADDR_ANY      (0x00000000)
#endif
#ifndef INADDR_NONE
    #define INADDR_NONE     (0xffffffff)
#endif


class NetworkEngine;
class NetworkEngineThread;
class NetworkEngineAccept;
class NetworkEngineRecv;
class NetworkEngineSend;

//namespace Network {
    enum MessageTypes {NewConnection, Disconnection, DataIncoming, DataOutgoing, DNSLookup};
//}
typedef enum net::MessageTypes MessageType;

typedef uint32_t ConnectionID;
const ConnectionID MaxConnectionID = std::numeric_limits<ConnectionID>::max();
const ConnectionID InvalidConnectionID = 0;


// The format for all communication of data/connections with/from NetworkEngine.
// cid = A run-time unique ID for a connection.
// status = The status of the connection from the senders point of view.
// size = The size of the data transfered by the message.
// data = A pointer to the data buffer.
class NetworkMessage {

public:
    NetworkMessage(ConnectionID c, net::MessageType t, std::size_t s = 0, char* d = NULL);

// FIXME: Handle NetworkMessage object construct/destruct with a pool allocator.
    static NetworkMessage* construct(void);
    static NetworkMessage* construct(ConnectionID c, net::MessageType t, std::size_t s = 0, char* d = NULL);
    static void            destruct(NetworkMessage* sd);
    ConnectionID cid;
    net::MessageType type;
    std::chrono::steady_clock::time_point received_at;
    std::size_t size;
    char*  data;

private:
    NetworkMessage(void);
};

inline NetworkMessage::NetworkMessage(ConnectionID c, net::MessageType t, std::size_t s, char* d) :
    cid(c), type(t), received_at(std::chrono::steady_clock::now()), size(s), data(d) {
}
//enum NetworkMessageTypes {NewConnection, Disconnection, DataIncoming, DataOutgoing, DNSLookup};

inline NetworkMessage* NetworkMessage::construct(ConnectionID c, net::MessageType t, std::size_t s, char* d) {
    assert(c != InvalidConnectionID);
    #ifdef DEBUG
        switch(t) {
        case net::MessageTypes::NewConnection:
        case net::MessageTypes::Disconnection:
            if (d == NULL) {
                assert(s == 0);
            }
            break;
        case net::MessageTypes::DataIncoming:
            assert(s > 0);
            assert(d != NULL);
            break;
        case net::MessageTypes::DataOutgoing:
            assert(s > 0);
            assert(d != NULL);
            break;
        case net::MessageTypes::DNSLookup:
            assert(s > 0);
            assert(d != NULL);
            break;
        }
    #endif
    return new NetworkMessage(c, t, s, d);
}

inline void NetworkMessage::destruct(NetworkMessage* m) {
    delete[] m->data;
    delete m;
}


// TODO: Add IP, port and hostname data fields?
//       GameManager will want to get that data, but we don't really have any use for it here so lets keep
//       things as lean and mean as possible and skip storing that data (it will be retrieved once for a
//       new connection so the information can be logged).
class SocketData {
public:
    SocketData(ConnectionID c, SOCKET sock);

    // FIXME: Handle SocketData object construct/destruct with a pool allocator.
    static SocketData* construct(ConnectionID c, SOCKET sock);
    static void        destruct(SocketData* sd);

    ConnectionID cid;
    SOCKET s;
    uint64_t rx;
    uint64_t tx;
};

inline SocketData::SocketData(ConnectionID c, SOCKET sock) : cid(c), s(sock), rx(0), tx(0) {
    assert(c != InvalidConnectionID);
    assert(s != INVALID_SOCKET);
}

inline SocketData* SocketData::construct(ConnectionID c, SOCKET sock) {
    assert(c != InvalidConnectionID);
    assert(sock != INVALID_SOCKET);
    return new SocketData(c, sock);
}
inline void SocketData::destruct(SocketData* sd) {
    delete sd;
}


} // namespace net


// A thread-safe array with its elements sorted based on their memory address location, descending sort-
// order. Perfect for rarely changing data sets which gets processed sequentially frequently. If used with
// a custom allocator for the elements it can give "perfect" cache/stream utilization and performance.
class MemoryLocationSortedArrayMT {


};


#include "UnorderedQueueMT.h"

typedef UnorderedQueueMT<net::SocketData*> SocketQueue;
typedef UnorderedQueueMT<net::NetworkMessage*> NetworkQueue;


#endif // NETWORKCORE_H
