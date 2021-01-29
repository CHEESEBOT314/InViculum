#ifndef INVICULUM_MODULE_HPP
#define INVICULUM_MODULE_HPP

#include <cstdint>

namespace modules {
    class module {
    public:
        virtual void render() = 0;
        virtual void move_player(float x, float y) = 0;

        uint32_t shader = 0;
    };
}


#endif //INVICULUM_MODULE_HPP
