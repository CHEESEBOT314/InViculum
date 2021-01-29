#ifndef INVICULUM_GLFWWRAPPER_HPP
#define INVICULUM_GLFWWRAPPER_HPP

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

/**
 * This is a header file, please see source file in src/main instead
 */
namespace glfw_wrapper {
    std::vector<const char*> init(void (*func)(int, int));
    bool create_surface(const vk::Instance& instance, vk::SurfaceKHR& surface_khr);

    void get_resolution(int* width, int* height);

    void poll_events();
    double get_time();

    bool should_quit();
    void terminate();
}

#endif//INVICULUM_GLFWWRAPPER_HPP
