#include <modules/directional_light.hxx>

#include <vml/transform.hxx>
#include <render/render_manager.hxx>
#include <cmath>

const float PI = 3.1415926535897932384f;

namespace modules {
    /**
     * direction light - Direction Light Constructor sets up the two planes where shadows are projected onto
     * @param left - left bound -x
     * @param right - right bound +x
     * @param up - upper bound +y
     * @param front - front bound +z
     * @param back - back bound -z
     * @param ld - light distance
     */
    directional_light::directional_light(float left, float right, float up, float front, float back, float ld) : l(left), r(right), u(up), f(front), b(back), light_distance(ld), light_angle(0.0F) {
        // Create the back plane by transforming (0, 0, 0), (1, 0, 0), (0, 1, 0) and (1, 1, 0) to (l, 0, b), (r, 0, b), (l, u, b) and (r, u, b) respectively
        bw = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, u, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, l, 0.0F, b, 1.0F);
        // Create the floor plane by transforming (0, 0, 0), (1, 0, 0), (0, 1, 0) and (1, 1, 0) to (l, 0, f), (r, 0, f), (l, 0, b) and (r, 0, b) respectively
        fl = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, b - f, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, l, 0.0F, f, 1.0F);
    }
    /**
     * render - Render function renders all components present in this module
     */
    void directional_light::render() {

        // Centre on the 'player'
        render::render_manager::set_view(vml::translate(vml::vec3(-player_pos[0], -player_pos[1], 0.0F)));

        // Calculate the light direction from the light_angle and light_distance and send it to the shader
        vml::vec3 light = vml::vec3(light_distance * (float)std::sin(light_angle * PI / 2.0F), -light_distance * (float)std::cos(light_angle * PI / 2.0F), -light_distance * 1.5F);
        render::render_manager::set_light_dir(light);

        // Render the back plane
        render::render_manager::set_model(bw);
        render::render_manager::draw_rect_2D();
        // Render the floor plane
        render::render_manager::set_model(fl);
        render::render_manager::draw_rect_2D();

        // Set player colour to red
        static vml::mat4 player = vml::mat4::identity();
        player[0][0] = 1.0F; player[1][1] = 0.25F; player[2][2] = 0.25F;
        render::render_manager::set_colour_mult(player);
        // Calculate the model matrix for the player given the position and render the player
        pt = vml::mat4(0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, player_pos[0] - 0.25F, player_pos[1] - 0.25F, (b + f) / 2.0F, 1.0F);
        render::render_manager::set_model(pt);
        render::render_manager::draw_rect_2D();

        // Calculate the projections of the light onto each surface
        bwP = vml::directional_project_z(light, b + 0.0001F);
        flP = vml::directional_project_y(light, 0.0001F);

        // Set shadow colour to black with partial transparency ( this value can be changed to create a different feel )
        static vml::mat4 shadow = vml::mat4::identity();
        shadow[0][0] = shadow[1][1] = shadow[2][2] = 0.0F;
        shadow[3][3] = 0.2F;
        render::render_manager::set_colour_mult(shadow);
        render::render_manager::set_is_shadow(true);

        // Draw the shadow on the back plane
        render::render_manager::set_model(bwP * pt);
        render::render_manager::draw_rect_2D();

        // Draw the shadow on the floor plane
        render::render_manager::set_model(flP * pt);
        render::render_manager::draw_rect_2D();
    }

    /**
     * move_player - Move Player function changes how the scene is displayed
     * @param x - changes the x position of the player
     * @param y - changes the light angle
     */
    void directional_light::move_player(float x, float y) {
        player_pos[0] += x;
        // Bound between (l + 0.25 and r - 0.25)
        if (player_pos[0] > r - 0.25F) {
            player_pos[0] = r - 0.25F;
        }
        else if (player_pos[0] < l + 0.25F) {
            player_pos[0] = l + 0.25F;
        }
        light_angle += y;
        // Bound between (l + 0.25 and r = 0.25)
        if (light_angle < -0.9F) {
            light_angle = -0.9F;
        }
        else if (light_angle > 0.9F) {
            light_angle = 0.9F;
        }
    }
}