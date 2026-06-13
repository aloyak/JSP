#pragma once

#include "engine/render/render.h"
#include "engine/components/cameraComponent.h"
#include "engine/components/entity.h"

inline void DrawGrid(Renderer& renderer, Entity* m_camera) {
    for (int i = -13; i <= 13; ++i) {
        Vec3 start = Vec3(i * 750.0f, 0, -9750.0f);
        Vec3 end   = Vec3(i * 750.0f, 0,  9750.0f);
        renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(),
                          m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
        start = Vec3(-9750.0f, 0, i * 750.0f);
        end   = Vec3( 9750.0f, 0, i * 750.0f);
        renderer.drawLine(start, end, m_camera->getComponent<CameraComponent>()->getCamera(),
                          m_camera->transform, Vec3(.005f, .005f, .005f), 10.0f, false);
    }
}