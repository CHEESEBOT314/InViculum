#ifndef INVICULUM_MULTI_POINT_LIGHT_HPP
#define INVICULUM_MULTI_POINT_LIGHT_HPP

#include <modules/module.hxx>

#include <vml/mat4.hxx>
#include <cstdint>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace modules {
    class multi_point_light : public module {
    public:
        multi_point_light(float left, float right, float up, float down, float front, float back);

        void render() override;
        void move_player(float x, float y) override;

    private:
        float l, r, u, d, f, b;
        vml::mat4 lw, rw, bw, fl, ce, pt;
        vml::vec3 light0, light1;
        vml::mat4 lwP0, rwP0, bwP0, flP0, ceP0, lwP1, rwP1, bwP1, flP1, ceP1;
        vml::vec2 player_pos = vml::vec2(0.0F, 0.0F);
    };
}

#endif //INVICULUM_MULTI_POINT_LIGHT_HPP
