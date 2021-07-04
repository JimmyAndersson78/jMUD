/******************************************************************************
 * file: NetworkEngine.h
 *
 * created: 2012-02-20 by jimmy
 * authors: jimmy
 *****************************************************************************/
// TODO: Remodel
//
//       1) update loop checking for incoming data
//          a) read incoming data
//          b) add incoming data to a local queue
//          c) use call-back to hand over local queue of incoming data
//
//       2) update loop checking queue for outgoing data
//          a) send all outgoing data
//              a.1) if not all outgoing data could be sent keep the rest
//          b) delete outgoing data from queue if all was sent
//
//       3) update loop checking for new connections
//          a) accept new connection and pass along a message through the call-back
//
//       4) update loop checking for disconnect messages
//          a) disconnect IDed connection
//
//       Basically it is intended to not have to have any knowledge at all about what is using it, so it is
//       using call-back functions passing along messages with input data, accepting messages about output
//       data and doing similar things for connection status updates (new/closed connections).
//
#ifndef NETWORKENGINE_H
#define NETWORKENGINE_H

// Debug related configuration options.
#ifndef NETWORK_DATA_VERIFICATION
    #define NETWORK_DATA_VERIFICATION_NORMAL        0
    #define NETWORK_DATA_VERIFICATION_PARANOID      1

    #define NETWORK_DATA_VERIFICATION               NETWORK_DATA_VERIFICATION_NORMAL
#endif

#include "config.h"
#include "NetworkCore.h"

#include "sys/socket.h" // SOMAXCONN
#include <thread>       // std::thread
#include <list>         // std::list<T>
#include <stack>        // std::stack<T>
#include <cassert>      // assert()


namespace net {


/***
 * The communications class, it is used to initialize communication and to
 * perform the actual reading and sending of data to the players.
 */
class NetworkEngine {
    friend class NetworkEngineThread;
    friend class NetworkEngineAccept;
    friend class NetworkEngineRecv;
    friend class NetworkEngineSend;

public:
    // NOTE: NetworkEngine run-time (non-)configurable settings.
    static const bool use_dns_lookup = false;
    static const bool use_ipv4 = true;
    static const bool use_ipv6 = true;
    static const bool use_strict_bind = false;

    static const std::size_t MaxConnectionsQueued = 128;
    static const std::size_t MaxSocketsPerThread = 512;
    static const std::size_t SocketsPerThreadHigh = MaxSocketsPerThread - 10;
    static const std::size_t SocketsPerThreadLow = MaxSocketsPerThread * 0.75;
    static const std::size_t SocketServerOptionListenQueueLength = SOMAXCONN;
//    static const std::size_t SocketServerOptionListenQueueLength = 64;

    //
    static NetworkEngine& instance(void);

    // Control methods.
    bool initialize(int maxConnections, IPPort port, const char *ipv4, const char *ipv6);   // Initializes communication subsystems.
    bool close(void);        // Flushes all output and closes connections.

    // Information methods.
    unsigned int GetNumConnections(void) {return users_current;}
    unsigned int GetPeakConnections(void) {return users_peak;}
    unsigned int GetTotalConnections(void) {return users_total;}
    std::size_t  GetMaxConnectionsTotal(void) {return _MaxConnectionsTotal;}

    uint64_t GetBytesRecv(void) {return rx_bytes;}
    uint64_t GetBytesSend(void) {return tx_bytes;}

    bool empty(void) {if (users_current != 0) return false; return true;}

    void LogStatus(void);

    void AddNewConnection(SOCKET s);                    //
    void DisconnectConnection(SocketData* sd);          //

    void QueueSendMessage(NetworkMessage* m);
    void QueueRecvMessage(NetworkMessage* m);

    // Socket control methods.
    static SOCKET socket_create(int type);
    static SOCKET setup_server_socket(int type, const char* host, IPPort port);// Setup a socket for an accept-thread
    static bool   socket_bind(SOCKET s, int ai_family, const char *bindaddr, IPPort port);
    static void   socket_close(SOCKET s);               // closes socket

    static bool   socket_mode_listen(SOCKET s, int n);  // listen mode
    static bool   socket_mode_nonblocking(SOCKET s);    // non-blocking mode
    static bool   socket_mode_reuseaddr(SOCKET s);      // reuse address mode
    static bool   socket_mode_linger(SOCKET s);         // linger mode
    static bool   socket_mode_ipv6only(SOCKET s);       // IPv6 only (no IPv4 on same socket)
    static bool   socket_mode_keepalive(SOCKET s);      // send keepalive packets
    static bool   socket_mode_timestamp(SOCKET s);      // enable timestamps
    static bool   socket_mode_nodelay(SOCKET s);        // disables TCP packet concatenation

    static long   socket_send(SOCKET s, const char *data, std::size_t length);    // write to socket
    static long   socket_read(SOCKET s, char *data, std::size_t length);          // read from socket

    static int         get_error_code(void);        // Get the last error code.
    static const char* get_error_msg(int e = 0);    // Get the string desc for the given, or last, error code.
    bool               shutdown(void) {return _shutdown;}
    bool               terminate(void) {return _terminate;}

private:
    NetworkEngine(void);
    ~NetworkEngine(void);

    NetworkEngine(const NetworkEngine&);
    NetworkEngine& operator=(const NetworkEngine&);

    void SpawnAcceptThread(const char* name, const char* addr, IPPort port, int type);
    void SpawnRecvThread(const char* name);
    void SpawnSendThread(const char* name);

    std::list<NetworkEngineThread*> threads;
    std::mutex mutex_threads;

    char*  server_hostname;
    IPPort server_port;
    bool   _shutdown;
    bool   _terminate;

    ConnectionID nextCID;

    unsigned int users_total;    // Total number of connections.
    unsigned int users_current;  // Number of connections currently.
    unsigned int users_peak;     // Max number of connections at one time.
    std::size_t  _MaxConnectionsTotal;

    // Work queues for new and disconnected users.
//    UnorderedQueueMT<SocketData*> uqueue_new;
//    UnorderedQueueMT<SocketData*> uqueue_remove;
//    UnorderedQueueMT<NetworkMessage*> messagesToSend;
    SocketQueue uqueue_new;
    SocketQueue uqueue_remove;
    NetworkQueue messagesToSend;


    // Statistics
    // TODO: Package all statistics variables into a struct?
    //       That way we could return a const reference to it for interested parties, instead of having to
    //       request them one by one. And we could overload a stream output operator for the struct to
    //       simplify printing the statistics.
    static uint64_t rx_bytes;
    static uint64_t tx_bytes;
    static uint64_t nsocket_recv;
    static uint64_t nsocket_send;
    static uint32_t nsocket_accept;

    uint32_t threads_accept;
    uint32_t threads_recv;
    uint32_t threads_send;
};


inline NetworkEngine& NetworkEngine::instance(void) {
    static NetworkEngine instanceOfNetworkEngine;
    return instanceOfNetworkEngine;
}


} // namespace net

#endif // NETWORKENGINE_H
