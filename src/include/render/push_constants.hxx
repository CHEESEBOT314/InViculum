#ifndef INVICULUM_RENDER_PUSHCONSTANTS_HPP
#define INVICULUM_RENDER_PUSHCONSTANTS_HPP

#include <vml/mat4.hxx>
#include <vml/vec3.hxx>

namespace render {
    /**
     * push_constants - Push Constants structure is used to send key information to the shader pipeline:
     * projection matrix, view matrix, model matrix, colour multiplier matrix, light direction vector,
     * light 0 position, light 1 position and whether to render as a shadow
     */
    struct push_constants {
        vml::mat4 p;
        vml::mat4 v;
        vml::mat4 m;
        vml::mat4 cm;
        vml::vec4 light_dir;
        vml::vec4 light0;
        vml::vec4 light1;
        bool is_shadow;
    };
}

#endif//INVICULUM_RENDER_PUSHCONSTANTS_HPP
