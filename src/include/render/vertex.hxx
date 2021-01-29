#ifndef INVICULUM_VERTEX_HPP
#define INVICULUM_VERTEX_HPP

#include <vml/vec3.hxx>

namespace render {
    /**
     * vertex - Vertex structure used to store the key value about a vertex (position and uv)
     */
    struct vertex {
        vml::vec2 pos;
        vml::vec2 uv;
    };
}

#endif //INVICULUM_VERTEX_HPP
