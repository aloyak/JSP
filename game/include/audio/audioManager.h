#pragma once

#include "engine/audio/audio.h"

#include <string>
#include <unordered_set>

class AudioSystem;

enum AudioContext {
    Music,
    SFX,
    UserInterface,
    Master
};

struct AudioClip {
    std::string path;
    AudioContext context;
};

class AudioManager {
public:
    using SoundHandle = unsigned int;

    void update(float deltaTime) {
        if (m_audioSystem) {
            m_audioSystem->update(deltaTime);
        }
        pruneFinishedHandles();
    }

    SoundHandle playSound(const std::string& soundPath, AudioContext context = Master, bool loop = false) {
        if (!m_audioSystem) {
            return 0;
        }

        const SoundHandle handle = m_audioSystem->playSound(soundPath, getVolume(context), loop);
        if (handle != 0 && context != Master) {
            trackHandle(handle, context);
        }
        return handle;
    }

    SoundHandle playMusic(const std::string& soundPath, bool loop = true, float fadeInDuration = 0.0f) {
        if (!m_audioSystem) {
            return 0;
        }

        if (m_currentMusicHandle != 0) {
            m_audioSystem->stopSound(m_currentMusicHandle);
            m_currentMusicHandle = 0;
        }

        const float targetVolume = getVolume(Music);
        const float startVolume = fadeInDuration > 0.0f ? 0.0f : targetVolume;

        m_currentMusicHandle = m_audioSystem->playSound(soundPath, startVolume, loop);
        m_currentMusicPath = soundPath;

        if (m_currentMusicHandle != 0 && fadeInDuration > 0.0f) {
            m_audioSystem->fadeSoundVolume(m_currentMusicHandle, targetVolume, fadeInDuration);
        }

        return m_currentMusicHandle;
    }

    void stopMusic(float fadeOutDuration = 0.0f) {
        if (!m_audioSystem || m_currentMusicHandle == 0) {
            return;
        }

        if (fadeOutDuration > 0.0f) {
            m_audioSystem->fadeOutAndStop(m_currentMusicHandle, fadeOutDuration);
        } else {
            m_audioSystem->stopSound(m_currentMusicHandle);
        }

        m_currentMusicHandle = 0;
        m_currentMusicPath.clear();
    }

    bool isMusicPlaying() const {
        return m_currentMusicHandle != 0 && m_audioSystem && m_audioSystem->isHandleValid(m_currentMusicHandle);
    }

    const std::string& getCurrentMusicPath() const { return m_currentMusicPath; }

    void setVolume(float volume, AudioContext context = Master) {
        switch (context) {
            case Music:  m_musicVolume = volume; break;
            case SFX:    m_sfxVolume = volume; break;
            case UserInterface:     m_uiVolume = volume; break;
            case Master: m_masterVolume = volume; break;
            default:     break;
        }

        if (!m_audioSystem) {
            return;
        }

        if (context == Master) {
            m_audioSystem->setGlobalVolume(volume);
            return;
        }

        if (context == Music) {
            if (m_currentMusicHandle != 0) {
                m_audioSystem->setSoundVolume(m_currentMusicHandle, volume);
            }
            return;
        }

        for (SoundHandle handle : m_sfxHandles) {
            m_audioSystem->setSoundVolume(handle, volume);
        }
    }

    float getVolume(AudioContext context = Master) const {
        switch (context) {
            case Music:  return m_musicVolume;
            case SFX:    return m_sfxVolume;
            case UserInterface:     return m_uiVolume;
            case Master: return m_masterVolume;
            default:     return 1.0f;
        }
    }

    void setAudioSystem(AudioSystem* audioSystem) {
        m_audioSystem = audioSystem;
        if (m_audioSystem) {
            m_audioSystem->setGlobalVolume(m_masterVolume);
        }
    }

private:
    void trackHandle(SoundHandle handle, AudioContext context) {
        if (context == SFX) {
            m_sfxHandles.insert(handle);
        }
    }

    void pruneFinishedHandles() {
        if (!m_audioSystem) {
            return;
        }

        for (auto it = m_sfxHandles.begin(); it != m_sfxHandles.end();) {
            if (!m_audioSystem->isHandleValid(*it)) {
                it = m_sfxHandles.erase(it);
            } else {
                ++it;
            }
        }
    }

    float m_musicVolume = 1.0f;
    float m_sfxVolume = 1.0f;
    float m_uiVolume = 0.25f;
    float m_masterVolume = 1.0f;

    AudioSystem* m_audioSystem = nullptr;

    std::unordered_set<SoundHandle> m_sfxHandles;

    SoundHandle m_currentMusicHandle = 0;
    std::string m_currentMusicPath;
};