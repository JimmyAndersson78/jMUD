#ifndef ENTITY_H
#define ENTITY_H

#include "config.h"



typedef uint32_t EntityID;
class EntityManager;


class Entity {
  friend class EntityManager;

  public:
    EntityID getID(void) {return id;}

  private:
    Entity(EntityID _id) : id(_id) {}
    Entity(const Entity& e);
    ~Entity() {}
    Entity& operator=(const Entity&);

    EntityID id;
};

#endif // ENTITY_H
