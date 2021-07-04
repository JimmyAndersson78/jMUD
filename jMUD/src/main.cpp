#include "Settings.h"
#include "server/GameServer.h"


class Settings settings("settings.ini");


int main(int argc, char* argv[]) {
    GameServer game;
    return game.run(argc, argv);
}
