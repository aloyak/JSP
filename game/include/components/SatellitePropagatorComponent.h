#pragma once

#include "engine/components/component.h"
#include "simulation/satelliteManager.h"

#include <SGP4.h>
#include <Tle.h>
#include <Eci.h>
#include <DateTime.h>

class SatellitePropagatorComponent : public Component {
private:
    libsgp4::Tle m_tle;
    libsgp4::SGP4 m_sgp4;

public:
    SatellitePropagatorComponent(const Satellite& data) 
        : m_tle(data.name, data.line1, data.line2), 
          m_sgp4(m_tle) 
    {}

    Vec3 GetPositionECI(const libsgp4::DateTime& date) {
        libsgp4::Eci eci = m_sgp4.FindPosition(date);
        return Vec3(eci.Position().x, eci.Position().y, eci.Position().z);
    }

    std::unique_ptr<Component> clone() const override {
        return std::make_unique<SatellitePropagatorComponent>(*this);
    }
};