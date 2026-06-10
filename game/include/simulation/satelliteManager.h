#pragma once

#include <string>
#include <vector>
#include "engine/core/math.h"
#include "engine/utils/path.h"

struct Satellite {
    std::string name;
    unsigned int noradID;
    double epochYear;
    double epochDays;
    double bstar;
    double inclination;
    double raan;
    double eccentricity;
    double argOfPerigee;
    double meanAnomaly;
    double meanMotion;
    std::string line1;
    std::string line2;
};

class SatelliteManager {
private:
    std::string m_cachePath = "cache/satellites.json";
    std::vector<Satellite> m_satellites;
    
    bool isCacheValid();
    bool fetchSatelliteData();
    void loadFromCache();
    void parseJson(const std::string& jsonData);

public:
    void initialize();
    const std::vector<Satellite>& getSatellites() const { return m_satellites; }
};