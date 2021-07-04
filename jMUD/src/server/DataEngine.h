#ifndef DATAENGINE_H
#define DATAENGINE_H

#include "config.h"
#include "Player.h"
#include "network/NetworkCore.h"

#include <list>


typedef std::list<Player*> PlayerList;



class DataEngine {
public:
    static DataEngine &instance(void);
    bool initialize(void);

    bool AddPlayer(net::ConnectionID c);
    bool RemPlayer(net::ConnectionID c);

    std::size_t GetNumPlayers(void);

  private:
    DataEngine(void);
    DataEngine(const DataEngine&);
    ~DataEngine(void);

    ObjectID GetNewID(void);

    PlayerList::iterator FindPlayerByCID(net::ConnectionID cid);
    PlayerList::iterator FindPlayerByOID(ObjectID oid);

    ObjectID nextID;
    PlayerList players;
};

inline DataEngine& DataEngine::instance () {
    static DataEngine instanceOfDataEngine;
    return instanceOfDataEngine;
}

inline ObjectID DataEngine::GetNewID(void) {
    return nextID++;
}

inline std::size_t DataEngine::GetNumPlayers(void) {
    return players.size();
}
#endif // DATAENGINE_H
