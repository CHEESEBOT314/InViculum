#ifndef INVICULUM_SINGLE_POINT_LIGHT_HPP
#define INVICULUM_SINGLE_POINT_LIGHT_HPP

#include <modules/module.hxx>

#include <vml/mat4.hxx>
#include <cstdint>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace modules {
    class single_point_light : public module {
    public:
        single_point_light(float left, float right, float up, float down, float front, float back);

        void render() override;
        void move_player(float x, float y) override;

    private:
        float l, r, u, d, f, b;
        vml::mat4 lw, rw, bw, fl, ce, pt;
        vml::vec3 light;
        vml::mat4 lwP, rwP, bwP, flP, ceP;
        vml::vec2 player_pos = vml::vec2(0.0F, 0.0F);
    };
}

#endif //INVICULUM_SINGLE_POINT_LIGHT_HPP
