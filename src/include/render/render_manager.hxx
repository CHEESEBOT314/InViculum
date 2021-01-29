#ifndef INVICULUM_RENDER_RENDERMANAGER_HPP
#define INVICULUM_RENDER_RENDERMANAGER_HPP

#include <string>
#include <vml/mat4.hxx>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace render::render_manager {
    void init();

    bool create_graphics_pipeline(const std::string& name);
    uint32_t get_pipeline(const std::string& name);

    void bind_pipeline(uint32_t id);

    float get_aspect_ratio();

    void reset_push_constants();
    void set_perspective(const vml::mat4& pers);
    void set_view(const vml::mat4& view);
    void set_model(const vml::mat4& mode);
    void set_colour_mult(const vml::mat4& cm);
    void set_light_dir(const vml::vec3& l);
    void set_light0(const vml::vec3& l);
    void set_light1(const vml::vec3& l);
    void set_is_shadow(bool is);

    void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
    void draw_rect_2D();

    bool load_shaders();
    void unload_shaders();
    bool reload_shaders();


    void terminate();
}

#endif//INVICULUM_RENDER_RENDERMANAGER_HPP
