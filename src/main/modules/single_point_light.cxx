#include <modules/single_point_light.hxx>

#include <vml/transform.hxx>
#include <render/render_manager.hxx>

namespace modules {
    /**
     * single_point_light - Single Point Light constructor sets up the required surfaces, light and projections for the scene
     * @param left - left bound x-
     * @param right - right bound x+
     * @param up - up bound y+
     * @param down - down bound y-
     * @param front - front bound z+
     * @param back - back bound z-
     */
    single_point_light::single_point_light(float left, float right, float up, float down, float front, float back) : l(left), r(right), u(up), d(down), f(front), b(back) {
        // Construct matrices to represent each surface of the room
        lw = vml::mat4(0.0F, 0.0F, b - f, 0.0F, 0.0F, u - d, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, l, d, f, 1.0F);
        rw = vml::mat4(0.0F, 0.0F, f - b, 0.0F, 0.0F, u - d, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F, r, d, b, 1.0F);
        bw = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, u - d, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, l, d, b, 1.0F);
        fl = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, b - f, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, l, d, f, 1.0F);
        ce = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, f - b, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, l, u, b, 1.0F);
        // Place the light at a suitable location
        light = vml::vec3((r + l) / 2.0F, (u * 3.0F + d) / 4.0F, f);
        // Calculate the projection matrices for the light onto each surface
        lwP = vml::point_project_x(light, l + 0.0001F);
        rwP = vml::point_project_x(light, r - 0.0001F);
        bwP = vml::point_project_z(light, b + 0.0001F);
        flP = vml::point_project_y(light, d + 0.0001F);
        ceP = vml::point_project_y(light, u - 0.0001F);
    }

    /**
     * render - Render function renders all components present in this module
     */
    void single_point_light::render() {
        // Send light position to the shader
        render::render_manager::set_light0(light);
        // Centre on the player
        render::render_manager::set_view(vml::translate(vml::vec3(-player_pos[0], -player_pos[1], 0.0F)));
        // Draw all surfaces
        render::render_manager::set_model(lw);
        render::render_manager::draw_rect_2D();
        render::render_manager::set_model(rw);
        render::render_manager::draw_rect_2D();
        render::render_manager::set_model(bw);
        render::render_manager::draw_rect_2D();
        render::render_manager::set_model(fl);
        render::render_manager::draw_rect_2D();
        render::render_manager::set_model(ce);
        render::render_manager::draw_rect_2D();
        // Set player colour to red
        static vml::mat4 player = vml::mat4::identity();
        player[0][0] = 1.0F; player[1][1] = 0.25F; player[2][2] = 0.25F;
        render::render_manager::set_colour_mult(player);
        // Calculate the model matrix for the player given the position and render the player
        pt = vml::mat4(0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, player_pos[0] - 0.25F, player_pos[1] - 0.25F, (3.0F * b + f) / 4.0F, 1.0F);
        render::render_manager::set_model(pt);
        render::render_manager::draw_rect_2D();

        // Set shadow colour to black with partial transparency ( this value can be changed to create a different feel )
        static vml::mat4 shadow = vml::mat4::identity();
        shadow[0][0] = shadow[1][1] = shadow[2][2] = 0.0F;
        shadow[3][3] = 0.2F;
        render::render_manager::set_colour_mult(shadow);
        render::render_manager::set_is_shadow(true);

        // Draw shadows on each plane only if the player is in a valid position to not cause weird projections
        // i.e the player must be on the correct side of the light for it to be drawn

        if (player_pos[0] + 0.25F < light[0]) {
            render::render_manager::set_model(lwP * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[0] - 0.25F > light[0]) {
            render::render_manager::set_model(rwP * pt);
            render::render_manager::draw_rect_2D();
        }
        render::render_manager::set_model(bwP * pt);
        render::render_manager::draw_rect_2D();
        if (player_pos[1] + 0.25F < light[1]) {
            render::render_manager::set_model(flP * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[1] - 0.25F > light[1]) {
            render::render_manager::set_model(ceP * pt);
            render::render_manager::draw_rect_2D();
        }
    }

    /**
     * move_player - Move Player function changes how the scene is displayed
     * @param x - changes the x position of the player
     * @param y - changes the y position of the player
     */
    void single_point_light::move_player(float x, float y) {
        player_pos[0] += x;
        player_pos[1] += y;
        //Bound x between (l + 0.25 and r - 0.25)
        if (player_pos[0] > r - 0.25F) {
            player_pos[0] = r - 0.25F;
        }
        else if (player_pos[0] < l + 0.25F) {
            player_pos[0] = l + 0.25F;
        }
        //Bound y between (d + 0.25 and u - 0.25)
        if (player_pos[1] > u - 0.25F) {
            player_pos[1] = u - 0.25F;
        }
        else if (player_pos[1] < d + 0.25F) {
            player_pos[1] = d + 0.25F;
        }
    }
}