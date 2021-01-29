#include "glfw_wrapper.hxx"

#include <memory>

namespace glfw_wrapper {
    namespace {
        // Holds all 'private' variables used by GLFW
        struct info {
            GLFWwindow *window;
            void (*key_func)(int, int);
        };
        std::unique_ptr<info> info_p;

        void error_callback(int error, const char* description) {
            printf("GLFW error %d: %s\n", error, description);
        }
        // Called when the user has pressed a key and the events have been polled
        void key_callback(GLFWwindow* w, int key, int scancode, int action, int mods) {
            // Close on escape
            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(w, true);
            }
            // Send key presses to game
            info_p->key_func(key, action);
        }
    }
    // Initialise GLFW by providing it with the event function
    std::vector<const char*> init(void (*func)(int, int)) {
        std::vector<const char*> extensions;
        if (!info_p) {
            glfwSetErrorCallback(error_callback);
            if (glfwInit() && glfwVulkanSupported()) {
                // Get all of the required Vulkan extensions for GLFW
                uint32_t ext_count;
                const char **ext_list = glfwGetRequiredInstanceExtensions(&ext_count);
                extensions = std::vector<const char*>(ext_list, ext_list + ext_count);
                glfwDefaultWindowHints();
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

                // Create window on the primary monitor
                GLFWwindow* w = glfwCreateWindow(vidmode->width, vidmode->height, "2D", monitor, nullptr);
                if (w) {
                    // If successful set everything else up
                    info_p = std::make_unique<info>();
                    info_p->window = w;
                    glfwSetKeyCallback(w, key_callback);
                    info_p->key_func = func;

                    return extensions;
                }
            }
        }
        extensions.clear();
        return extensions;
    }

    // Create the Vulkan surface using glfwCreateWindowSurface
    bool create_surface(const vk::Instance& instance, vk::SurfaceKHR& surface_khr) {
        if (info_p && info_p->window) {
            VkSurfaceKHR t_surf;
            VkResult result = glfwCreateWindowSurface(VkInstance(instance), info_p->window, nullptr, &t_surf);
            surface_khr = vk::SurfaceKHR(t_surf);
            return result == VK_SUCCESS;
        }
        return false;
    }

    // Get the resolution of the framebuffer (used by Vulkan to determine the swapchain size)
    void get_resolution(int* width, int* height) {
       glfwGetFramebufferSize(info_p->window, width, height);
    }

    // Poll all the user inputs
    void poll_events() {
        glfwPollEvents();
    }

    // Gets the time since GLFW was initialised
    double get_time() {
        return glfwGetTime();
    }

    // Determines if the application should exit, will return true when ESC is pressed
    bool should_quit() {
        return glfwWindowShouldClose(info_p->window) == GLFW_TRUE;
    }

    // Destroy all GLFW related objects
    void terminate() {
        if (info_p && info_p->window) {
            glfwDestroyWindow(info_p->window);
        }
        glfwTerminate();
        info_p.reset(nullptr);
    }
}

