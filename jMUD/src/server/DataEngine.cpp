
#include "config.h"
#include "DataEngine.h"
#include "log.h"




DataEngine::DataEngine() : nextID(1), players() {
}


DataEngine::~DataEngine() {
}


bool DataEngine::initialize(void) {
    log_INIT();

    sys::log::DataEngine::add("Doing nothing.");

    log_INIT_OK();
    return true;
}



bool DataEngine::AddPlayer(net::ConnectionID cid) {
    assert(cid != net::InvalidConnectionID);

    PlayerList::iterator existingPlayer = FindPlayerByCID(cid);
    if (existingPlayer != players.end()) {
        sys::log::DataEngine::warning("AddPlayer(): Attempt to add a second player with the same CID (%u).", cid);
        return false;
    }

    // FIXME: Properly initialize a new player.
    Player* player = new Player();
    player->SetID(GetNewID());
    player->SetCID(cid);
    players.push_back(player);
    return true;
}


bool DataEngine::RemPlayer(net::ConnectionID cid) {
    assert(cid != net::InvalidConnectionID);

    PlayerList::iterator existingPlayer = FindPlayerByCID(cid);
    if (existingPlayer == players.end()) {
        sys::log::DataEngine::warning("RemPlayer(): Attempt to remove a player with a CID (%u) that didn't exist.", cid);
        return false;
    }

    // FIXME: Persist the player and everything else one might want do to a player after it is disconnected.
    players.erase(existingPlayer);
//    delete (*existingPlayer);
    return true;
}


PlayerList::iterator DataEngine::FindPlayerByCID(net::ConnectionID cid) {
    assert(cid != net::InvalidConnectionID);

    PlayerList::iterator player = players.begin();
    for (; player != players.end(); ++player) {
        if ((*player)->GetCID() == cid)
            break;
    }
    return player;
}


PlayerList::iterator DataEngine::FindPlayerByOID(ObjectID oid) {
    assert(oid != InvalidObjectID);

    PlayerList::iterator player = players.begin();
    for (; player != players.end(); ++player) {
        if ((*player)->GetID() == oid)
            break;
    }
    return player;
}








