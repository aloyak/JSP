// USED FOR SANDBOX FOR NOW
// TODO: set all the builtin parameters

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include "engine/utils/logger.h"

struct GravityBody {
    std::string modelPath  = "assets/models/earth.fbx";
    float       mass       = 1.0f;
    float       radius     = 1.0f;
    float       period     = 24.0f;   // axial rotation period (game hours)
    std::string name       = "Unnamed";
    bool        isReal     = true;    // shows under the "Solar System" tab
    bool        isCustom   = false;   // loaded from a user .planet file
    std::string customPath = "";      // path to the source .planet JSON
};

class PlanetRegistry {
public:
    std::vector<GravityBody> bodies;

    PlanetRegistry() {

        bodies = {
            { "assets/models/planets/earth.fbx",    50,  637.1f,    24.0f, "Earth"   },
            { "assets/models/planets/moon.fbx",      7,  173.7f,    27.3f, "Moon"    },
            { "assets/models/planets/mars.fbx",     43,  338.9f,    24.6f, "Mars"    },
            { "assets/models/planets/venus.fbx",    23,  605.2f,   240.0f, "Venus"   },
            { "assets/models/planets/mercury.fbx",   5,  243.9f,  1408.0f, "Mercury" },
            { "assets/models/planets/jupiter.fbx", 132, 6991.1f,     9.9f, "Jupiter" },
            { "assets/models/planets/saturn.fbx",   56, 5823.2f,    10.7f, "Saturn"  },
            { "assets/models/planets/uranus.fbx",   75, 2536.2f,    17.2f, "Uranus"  },
            { "assets/models/planets/neptune.fbx",  17, 2462.2f,    16.1f, "Neptune" },
        };

    }

    const GravityBody* find(const std::string& name) const {
        for (const auto& b : bodies)
            if (b.name == name) return &b;
        return nullptr;
    }

    int loadCustom(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::error("PlanetRegistry: could not open '" + filePath + "'");
            return -1;
        }

        try {
            nlohmann::json j;
            file >> j;

            GravityBody body;
            body.modelPath  = "assets/models/planetbase.fbx";
            body.isReal     = false;
            body.isCustom   = true;
            body.customPath = filePath;

            if (j.contains("name"))   body.name   = j["name"].get<std::string>();
            if (j.contains("mass"))   body.mass   = j["mass"].get<float>();
            if (j.contains("radius")) body.radius = j["radius"].get<float>();
            if (j.contains("period")) body.period = j["period"].get<float>();

            bodies.push_back(body);
            return static_cast<int>(bodies.size()) - 1;

        } catch (...) {
            Logger::error("PlanetRegistry: failed to parse '" + filePath + "'");
            return -1;
        }
    }
};