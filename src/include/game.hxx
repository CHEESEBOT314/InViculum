#ifndef INVICULUM_GAME_HPP
#define INVICULUM_GAME_HPP

/**
 * This is a header file, please see source file in src/main instead
 */
namespace game {
    void init();

    void render();
    void handle_event(int key, int action);
    void update(double dt);

    bool should_quit();

    void terminate();
}

#endif//INVICULUM_GAME_HPP
