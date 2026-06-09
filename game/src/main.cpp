#include "engine/engine.h"
#include "engine/input/input.h"
#include "engine/scene/sceneManager.h"

#include "engine/utils/logger.h"

#include "engine/components/rendererComponent.h"
#include "engine/components/cameraComponent.h"
#include "engine/components/skyboxComponent.h"
#include "engine/components/directionalLightComponent.h"
#include "engine/components/pointLightComponent.h"
#include "engine/components/rigidbodyComponent.h"

#include <iostream>

int main() {
    Engine engine(1600, 900, "JSP");

    engine.getRenderer().setupRenderTarget(600, 400);
    engine.getRenderer().setPixelArt(true, 8);

    Input& input = engine.getInput();
    SceneManager& sm = engine.getSceneManager();

    input.setCursorMode(true); // true = locked
    engine.getWindow().setFullscreen(false);
    engine.getWindow().enableVSync(false);


    engine.run([&]() {
        
    });

    return 0;
}