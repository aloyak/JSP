#pragma once

#include <string>

class GameMode {
public:
    std::string modeName;
    GameMode(std::string name, const std::string& scenePath) : modeName(name), m_scenePath(scenePath) {}
    virtual ~GameMode() = default;

    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void Update() = 0;
    virtual void LateUpdate() {}

    const std::string& GetScenePath() const { return m_scenePath; }

private:
    std::string m_scenePath;
};