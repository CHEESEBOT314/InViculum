#include "game.hxx"
#include "glfw_wrapper.hxx"
#include "vulkan_wrapper.hxx"
#include "platform/platform.hxx"
#include "render/render_manager.hxx"
#include "resource/resource_manager.hxx"

/**
 * main - Entry point to the application.
 * @param argc - Argument count (unused)
 * @param args - Argument list (unused)
 * @return - Exit code
 */
int main(int argc, char** args) {
    // Retrieve Vulkan extensions required by glfw
    std::vector<const char*> extensions = glfw_wrapper::init(game::handle_event);
    if (extensions.empty()) {
        return 0;
    }
    // Create all Vulkan objects
    if (!vulkan_wrapper::create_instance(extensions) ||
        !vulkan_wrapper::create_surface(glfw_wrapper::create_surface, glfw_wrapper::get_resolution) ||
        !vulkan_wrapper::create_others()) {
        return 0;
    }
    // Initialise the resource manager to read binary files
    resource::resource_manager::init(platform::files::get_resource_folder(), platform::files::FILE_SEPARATOR);

    // Initialise the render manager and load all shaders
    render::render_manager::init();
    render::render_manager::load_shaders();

    // Initialise the game which includes all three example modules
    game::init();

    // Game loop, simple way to calculate the change in time to update the scenes precisely
    double old_time = glfw_wrapper::get_time();
    while (!(glfw_wrapper::should_quit() || game::should_quit())) {
        // Check for user inputs
        glfw_wrapper::poll_events();
        double new_time = glfw_wrapper::get_time();
        game::update(new_time - old_time);
        old_time = new_time;
        // Render the frame
        if (!vulkan_wrapper::render_frame(game::render)) {
            break;
        }
    }
    // Wait until all Vulkan processes have stoppped
    vulkan_wrapper::wait_idle();

    // Terminate everything
    render::render_manager::terminate();
    vulkan_wrapper::terminate();
    glfw_wrapper::terminate();
    return 0;
}

