#include <game.hxx>

#include <modules/multi_point_light.hxx>
#include <modules/single_point_light.hxx>
#include <modules/directional_light.hxx>
#include <render/render_manager.hxx>
#include <vml/transform.hxx>

#include <memory>
#include <GLFW/glfw3.h>

/**
 * game - Namespace which acts like a singleton class. Provides all functions to run the game.
 */
namespace game {
    /**
     * Anonymous namespace to hold 'private' variables
     */
    namespace {
        struct info {
            uint32_t dl_shader_id;
            uint32_t spl_shader_id;
            uint32_t mpl_shader_id;
            bool left;
            bool right;
            bool down;
            bool up;
            modules::directional_light* dl;
            modules::single_point_light* spl;
            modules::multi_point_light* mpl;

            modules::module* current;
        };
        std::unique_ptr<info> info_p;
    }
    /**
     * init - Initialisation function to load all necessary graphics pipelines and setup the example modules.
     */
    void init() {
        info_p = std::make_unique<info>();
        if (!render::render_manager::create_graphics_pipeline("multi_point") || !render::render_manager::create_graphics_pipeline("single_point") || !render::render_manager::create_graphics_pipeline("directional")) {
            return;
        }
        // Load example specific shader pipelines
        info_p->dl_shader_id = render::render_manager::get_pipeline("directional");
        info_p->spl_shader_id = render::render_manager::get_pipeline("single_point");
        info_p->mpl_shader_id = render::render_manager::get_pipeline("multi_point");
        // No arrow keys held by default
        info_p->left = false;
        info_p->right = false;
        info_p->down = false;
        info_p->up = false;
        // Create each scene and assign the correct shader pipeline to it
        info_p->dl = new modules::directional_light(-2.0F, 2.0F, 2.0F, -2.0F, -4.0F, 20.0F);
        info_p->dl->shader = info_p->dl_shader_id;
        info_p->spl = new modules::single_point_light(-2.0F, 2.0F, 1.0F, -1.0F, -2.0F, -4.0F);
        info_p->spl->shader = info_p->spl_shader_id;
        info_p->mpl = new modules::multi_point_light(-2.0F, 2.0F, 1.0F, -1.0F, -2.0F, -4.0F);
        info_p->mpl->shader = info_p->mpl_shader_id;

        // Set the current scene to the Directional Light example
        info_p->current = info_p->dl;
    }

    /**
     * render - Render function called from the game loop to render the currently selected example module.
     */
    void render() {
        render::render_manager::bind_pipeline(info_p->current->shader);
        render::render_manager::reset_push_constants();
        render::render_manager::set_perspective(vml::perspective(render::render_manager::get_aspect_ratio(), 0.5F, 0.1F, 10.0F));
        info_p->current->render();

    }

    /**
     * handle_event - Handle Event function used to change the example module shown and to change certain parameters.
     * @param key - Keyboard key used
     * @param action - Pressed or Released
     */
    void handle_event(int key, int action) {
        bool chosen = false;
        if (action == GLFW_PRESS) {
            chosen = true;
            // Change scene rendered base on the D, S and M keys
            if (key == GLFW_KEY_D) {
                info_p->current = info_p->dl;
                return;
            }
            if (key == GLFW_KEY_S) {
                info_p->current = info_p->spl;
                return;
            }
            if (key == GLFW_KEY_M) {
                info_p->current = info_p->mpl;
                return;
            }
        }
        else if (action == GLFW_RELEASE) {
            chosen = false;
        }
        else {
            return;
        }

        // Enabled/Disable directions using the arrow keys, used mainly for held movement
        if (key == GLFW_KEY_LEFT) {
            info_p->left = chosen;
        }
        else if (key == GLFW_KEY_RIGHT) {
            info_p->right = chosen;
        }
        else if (key == GLFW_KEY_DOWN) {
            info_p->down = chosen;
        }
        else if (key == GLFW_KEY_UP) {
            info_p->up = chosen;
        }
    }
    /**
     * update - Update function called every tick in order
     * @param dt - Delta time
     */
    void update(double dt) {
        // Update the certain parameter if the arrow key is held for all directions:
        // left = -x, right = +x, down = -y and up = +y
        if (info_p->left) {
            info_p->current->move_player(-(float)dt, 0.0F);
        }
        if (info_p->right) {
            info_p->current->move_player((float)dt, 0.0F);
        }
        if (info_p->down) {
            info_p->current->move_player(0.0F, -(float)dt);
        }
        if (info_p->up) {
            info_p->current->move_player(0.0F, (float)dt);
        }
    }

    // Enables the game to quit the application, not used
    bool should_quit() {
        return false;
    }

    // Free memory when the application closes
    void terminate() {
        info_p->current = nullptr;
        delete info_p->dl;
        delete info_p->spl;
        info_p.reset(nullptr);
    }
}