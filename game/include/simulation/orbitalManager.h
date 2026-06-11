#pragma once

#include "simulation/satelliteManager.h"
#include "components/SatellitePropagatorComponent.h"
#include "engine/engine.h"
#include "engine/components/entity.h"
#include "engine/components/rendererComponent.h"

#include <DateTime.h>

class OrbitalManager {
private:
    Engine& m_engine;
    SatelliteManager m_satelliteManager;
    std::vector<Entity*> m_orbitalEntities;
    Entity* m_earthEntity = nullptr;

    const float SCALE_FACTOR = 10.0f;

public:
    OrbitalManager(Engine& engine) : m_engine(engine) {}

    void initialize(Entity* earth) {
        m_earthEntity = earth;
        m_satelliteManager.initialize();

        for (const auto& data : m_satelliteManager.getSatellites()) {
            Entity* satEntity = m_engine.createEntity(data.name);
            satEntity->addComponent<SatellitePropagatorComponent>(data);
            satEntity->addComponent<RenderComponent>("assets/models/cube.fbx");
            satEntity->transform.scale = Vec3(0.1f, 0.1f, 0.1f);
            m_orbitalEntities.push_back(satEntity);
        }
    }

    void update() {
        if (!m_earthEntity) return;

        libsgp4::DateTime now = libsgp4::DateTime::Now(true);
        Vec3 earthPos = m_earthEntity->transform.position;

        for (Entity* sat : m_orbitalEntities) {
            auto propagator = sat->getComponent<SatellitePropagatorComponent>();
            if (propagator) {
                Vec3 posEci = propagator->GetPositionECI(now);

                sat->transform.position = Vec3(
                    earthPos.x + (posEci.x / SCALE_FACTOR),
                    earthPos.y + (posEci.y / SCALE_FACTOR),
                    earthPos.z + (posEci.z / SCALE_FACTOR)
                );
            }
        }
    }

    Entity* getEntityByIndex(size_t index) {
        if (index < m_orbitalEntities.size()) {
            return m_orbitalEntities[index];
        }
        return nullptr;
    }

    const std::vector<Satellite>& getSatelliteData() const {
        return m_satelliteManager.getSatellites();
    }
};