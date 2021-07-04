#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"
#include "network/NetworkCore.h"



class Player {
public:
    Player() : id(0), cid(0), status(0), state(0) {}
    bool operator==(Player& p);
    bool operator<(Player& p);

    net::ConnectionID GetCID(void);
    ObjectID          GetID(void);

    void SetCID(net::ConnectionID c);
    void SetID(ObjectID o);

private:
    ObjectID id;
    net::ConnectionID cid;

    int status;
    int state;
};


inline bool Player::operator==(Player& p) {
    if (p.id == id)
        return true;
    return false;
}


inline bool Player::operator<(Player& p) {
    if (p.id < id)
        return true;
    return false;
}


inline net::ConnectionID Player::GetCID(void) {
    return cid;
}


inline ObjectID Player::GetID(void) {
    return id;
}

inline void Player::SetCID(net::ConnectionID c) {
    cid = c;
}


inline void Player::SetID(ObjectID o) {
    if (id == 0) {
        id = o;
    }
}

#endif // PLAYER_H
