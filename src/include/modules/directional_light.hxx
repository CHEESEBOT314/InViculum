#ifndef INVICULUM_DIRECTIONAL_LIGHT_HPP
#define INVICULUM_DIRECTIONAL_LIGHT_HPP

#include <modules/module.hxx>
#include <vml/mat4.hxx>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace modules {
    class directional_light : public module {
    public:
        directional_light(float left, float right, float up, float front, float back, float ld);

        void render() override;
        void move_player(float x, float y) override;

    private:
        float l, r, u, f, b;
        vml::mat4 bw, fl, pt;
        float light_distance, light_angle;
        vml::mat4 bwP, flP;
        vml::vec2 player_pos = vml::vec2(0.0F, 0.25F);
    };
}

#endif //INVICULUM_DIRECTIONAL_LIGHT_HPP
