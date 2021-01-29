#include <modules/multi_point_light.hxx>

#include <vml/transform.hxx>
#include <render/render_manager.hxx>

namespace modules {
    /**
     * multi_point_light - Multi Point Light constructor sets up the required surfaces, lights and projections for the scene
     * @param left - left bound x-
     * @param right - right bound x+
     * @param up - up bound y+
     * @param down - down bound y-
     * @param front - front bound z+
     * @param back - back bound z-
     */
    multi_point_light::multi_point_light(float left, float right, float up, float down, float front, float back) : l(left), r(right), u(up), d(down), f(front), b(back) {
        // Construct matrices to represent each surface of the room
        lw = vml::mat4(0.0F, 0.0F, b - f, 0.0F, 0.0F, u - d, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, l, d, f, 1.0F);
        rw = vml::mat4(0.0F, 0.0F, f - b, 0.0F, 0.0F, u - d, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F, r, d, b, 1.0F);
        bw = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, u - d, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, l, d, b, 1.0F);
        fl = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, b - f, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, l, d, f, 1.0F);
        ce = vml::mat4(r - l, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, f - b, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, l, u, b, 1.0F);
        // Place light 0 at a suitable location
        light0 = vml::vec3((2.0F * r + l) / 3.0F, (u * 3.0F + d) / 4.0F, f);
        // Calculate the projection matrices for light 0 onto each surface
        lwP0 = vml::point_project_x(light0, l + 0.0001F);
        rwP0 = vml::point_project_x(light0, r - 0.0001F);
        bwP0 = vml::point_project_z(light0, b + 0.0001F);
        flP0 = vml::point_project_y(light0, d + 0.0001F);
        ceP0 = vml::point_project_y(light0, u - 0.0001F);
        // Place light 1 at a suitable location
        light1 = vml::vec3((r + 2.0F * l) / 3.0F, (u * 3.0F + d) / 4.0F, f);
        // Calculate the projection matrices for light 1 onto each surface
        lwP1 = vml::point_project_x(light1, l + 0.0002F);
        rwP1 = vml::point_project_x(light1, r - 0.0002F);
        bwP1 = vml::point_project_z(light1, b + 0.0002F);
        flP1 = vml::point_project_y(light1, d + 0.0002F);
        ceP1 = vml::point_project_y(light1, u - 0.0002F);
    }

    /**
     * render - Render function renders all components present in this module
     */
    void multi_point_light::render() {
        // Send light positions to the shader
        render::render_manager::set_light0(light0);
        render::render_manager::set_light1(light1);
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
        shadow[3][3] = 0.1F;
        render::render_manager::set_colour_mult(shadow);
        render::render_manager::set_is_shadow(true);

        // Draw shadows on each plane only if the player is in a valid position to not cause weird projections
        // i.e the player must be on the correct side of the light for it to be drawn

        if (player_pos[0] + 0.25F < light0[0]) {
            render::render_manager::set_model(lwP0 * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[0] - 0.25F > light0[0]) {
            render::render_manager::set_model(rwP0 * pt);
            render::render_manager::draw_rect_2D();
        }
        render::render_manager::set_model(bwP0 * pt);
        render::render_manager::draw_rect_2D();
        if (player_pos[1] + 0.25F < light0[1]) {
            render::render_manager::set_model(flP0 * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[1] - 0.25F > light0[1]) {
            render::render_manager::set_model(ceP0 * pt);
            render::render_manager::draw_rect_2D();
        }

        if (player_pos[0] + 0.25F < light1[0]) {
            render::render_manager::set_model(lwP1 * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[0] - 0.25F > light1[0]) {
            render::render_manager::set_model(rwP1 * pt);
            render::render_manager::draw_rect_2D();
        }
        render::render_manager::set_model(bwP1 * pt);
        render::render_manager::draw_rect_2D();
        if (player_pos[1] + 0.25F < light1[1]) {
            render::render_manager::set_model(flP1 * pt);
            render::render_manager::draw_rect_2D();
        }
        if (player_pos[1] - 0.25F > light1[1]) {
            render::render_manager::set_model(ceP1 * pt);
            render::render_manager::draw_rect_2D();
        }
    }

    /**
     * move_player - Move Player function changes how the scene is displayed
     * @param x - changes the x position of the player
     * @param y - changes the y position of the player
     */
    void multi_point_light::move_player(float x, float y) {
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