QT -= gui

CONFIG += c++20 console
CONFIG -= app_bundle

INCLUDEPATH += jMUD/src/utilities jMUD/src/config

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    jMUD/src/game/Component.cpp \
    jMUD/src/game/ComponentManager.cpp \
    jMUD/src/game/Entity.cpp \
    jMUD/src/game/EntityManager.cpp \
    jMUD/src/game/System.cpp \
    jMUD/src/game/SystemManager.cpp \
    jMUD/src/main.cpp \
    jMUD/src/server/DataEngine.cpp \
    jMUD/src/server/GameEngine.cpp \
    jMUD/src/server/GameServer.cpp \
    jMUD/src/server/Player.cpp \
    jMUD/src/server/network/NetworkEngine.cpp \
    jMUD/src/server/network/NetworkEngineAccept.cpp \
    jMUD/src/server/network/NetworkEngineRecv.cpp \
    jMUD/src/server/network/NetworkEngineSend.cpp \
    jMUD/src/server/world/WorldEngine.cpp \
    jMUD/src/server/world/WorldRoom.cpp \
    jMUD/src/server/world/WorldZone.cpp \
    jMUD/src/utilities/Settings.cpp \
    jMUD/src/utilities/gamelog.cpp \
    jMUD/src/utilities/log.cpp

TRANSLATIONS += \
    jMUD-workspace_en_US.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

HEADERS += \
    jMUD/src/config/config.h \
    jMUD/src/config/debug.h \
    jMUD/src/config/globals.h \
    jMUD/src/config/target.h \
    jMUD/src/game/Component.h \
    jMUD/src/game/ComponentManager.h \
    jMUD/src/game/Entity.h \
    jMUD/src/game/EntityManager.h \
    jMUD/src/game/System.h \
    jMUD/src/game/SystemManager.h \
    jMUD/src/server/DataEngine.h \
    jMUD/src/server/GameEngine.h \
    jMUD/src/server/GameServer.h \
    jMUD/src/server/Player.h \
    jMUD/src/server/network/NetworkCore.h \
    jMUD/src/server/network/NetworkEngine.h \
    jMUD/src/server/network/NetworkEngineAccept.h \
    jMUD/src/server/network/NetworkEngineRecv.h \
    jMUD/src/server/network/NetworkEngineSend.h \
    jMUD/src/server/network/NetworkEngineThread.h \
    jMUD/src/server/world/WorldEngine.h \
    jMUD/src/server/world/WorldRoom.h \
    jMUD/src/server/world/WorldZone.h \
    jMUD/src/server/world/world.h \
    jMUD/src/utilities/Settings.h \
    jMUD/src/utilities/UnorderedArray.h \
    jMUD/src/utilities/UnorderedQueueMT.h \
    jMUD/src/utilities/gamelog.h \
    jMUD/src/utilities/log.h
