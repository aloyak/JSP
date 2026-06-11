#pragma once

#include <string>

class GameMode {
public:
    GameMode(const std::string& scenePath) : m_scenePath(scenePath) {}
    virtual ~GameMode() = default;

    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void Update() = 0;
    virtual void LateUpdate() {}

    const std::string& GetScenePath() const { return m_scenePath; }

private:
    std::string m_scenePath;
};