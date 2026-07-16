#pragma once

#include "engine/utils/logger.h"
#include <string>

class Level {
public:
    Level(const std::string& name, const std::string& scenePath)
        : m_name(name), m_scenePath(scenePath) {}
    virtual ~Level() = default;
    
    virtual void OnEnter() = 0;
    virtual void Update() = 0;
    virtual void LateUpdate() = 0;
    virtual void OnExit() = 0;

    const std::string& GetName() const { return m_name; }
    const std::string& GetScenePath() const { return m_scenePath; }
private:
    std::string m_name;
    std::string m_scenePath;
};