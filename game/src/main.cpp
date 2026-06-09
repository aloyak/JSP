#include "engine/engine.h"

#include "game.h"
#include "gamemodes/explore.h"

int main() {
    Engine engine(1600, 900, "JSP");
    Game game(engine);

    game.SetGameMode(std::make_unique<ExploreMode>(game));

    engine.run([&]() {        
        game.Update();
    });

    return 0;
}