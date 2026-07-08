#include "engine/engine.h"

#include "game.h"
#include "gamemodes/mainmenu.h"

int main() {
    Engine engine(1920, 1080, "JSP");
    Game game(engine);

    game.SetGameMode(std::make_unique<MainMenuMode>(game, true), true, false);

    engine.run([&]() {        
        game.Update();
    },
    [&]() {
        game.LateUpdate();
    });

    return 0;
}