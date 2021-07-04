/*---------------------------------------------------------------------------*\
|* File: GameServer.h                                Part of jMUD-GameEngine *|
|*                                                                           *|
|* Description:                                                              *|
|*                                                                           *|
|*                                                                           *|
|* Authors: Jimmy Andersson <jimmy.andersson.1571@student.uu.se>             *|
|* Created: 2003-07-26 by Jimmy Andersson                                    *|
|* Version: 0.1.00                                                           *|
\*---------------------------------------------------------------------------*/
#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "config.h"
#include "network/NetworkCore.h"
#include "Player.h"

#include <stack>        // std::stack<T>
#include <mutex>        // std::mutex
#include <list>         // std::list<T>



/***
 * Main game server class. For setting up the different parts of the game,
 * closing them upon exit and calling them during runtime.
 */
class GameEngine {

  public:
    static GameEngine &instance(void);

    bool initialize(void);
    int  run(void);
    int  shutdown(int err = 0);

    uint64_t GetCycleCount(void) {return _cycle_count;}

    struct timeval GetTime(void) ;
    struct timeval GetCycleTime(void) {return _cycle_time;}
    inline time_t  GetBootTime(void) {return time_boot;}

    void LogSystemInfo(void);
    void LogSystemUsage(void);

    void AddMessageRecv(net::NetworkMessage* m);

  private:
    GameEngine(void);
    GameEngine(const GameEngine&);
    ~GameEngine(void);

    int update(void);

    void update_cycle(void);

    void sleep(unsigned int mseconds);
    void sleep(struct timespec t);

    bool runStatus;
    int errorCode;

    bool running;
    bool booted;

    uint64_t       _cycle_count;    //
    struct timeval _cycle_time;     // Gets updated on each heartbeat.
    time_t time_boot;               // Gets updated at boot.
    time_t time_now;                // Time "now".

    std::stack<net::NetworkMessage*> _network_io;  // Incoming data
    std::mutex mutex_network_io;

    std::list<Player*> _players;
};


inline GameEngine& GameEngine::instance () {
    static GameEngine instanceOfGameEngine;
    return instanceOfGameEngine;
}


inline void GameEngine::update_cycle(void) {
    ++_cycle_count;
    _cycle_time = GetTime();
}


inline void GameEngine::AddMessageRecv(net::NetworkMessage* m) {
    mutex_network_io.lock();
    _network_io.push(m);
    mutex_network_io.unlock();
}


#endif // GAMEENGINE_H
