#ifndef GAMESERVER_H
#define GAMESERVER_H



class GameServer {
  public:
    GameServer();
    ~GameServer();
    int run(int argc, char* argv[]);

  private:
    bool initialize();
    bool shutdown();
    void printHelp(void);

};

#endif // GAMESERVER_H
