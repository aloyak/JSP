#pragma once

#include "engine/components/entity.h"
#include "components/planetComponent.h"
#include "game.h"

#include <string>

struct Planet {
    std::string name;
    Entity* entity;
    PlanetComponent* component;
};

inline std::vector<Planet> GetPlanetsFromScene(Game& game, Scene* scene) {
    std::vector<Planet> planets;

    {
        Planet p;
        p.entity = scene->getEntityByName("Yin")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Yang")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Gaia")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("Kepler")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        planets.push_back(p);
    }
    {
        Planet p;
        p.entity = scene->getEntityByName("VestaStar")->get();
        p.name = p.entity->name;
        p.component = p.entity->addComponent<PlanetComponent>(game);
        planets.push_back(p);
    }

    return planets;
}