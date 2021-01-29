#ifndef INVICULUM_VML_TRANSFORM_HPP
#define INVICULUM_VML_TRANSFORM_HPP

#include <vml/mat4.hxx>
#include <vml/quaternion.hxx>

/**
 * This is a header file, please see source file in src/main instead
 */
namespace vml {
    mat3 scale(float s);
    mat3 scale(const vec3 &v);

    mat4 translate(const vec3 &v);
    mat4 translate(const mat3 &m, const vec3 &v);

    mat4 rotate_x(float rad);
    mat4 rotate_y(float rad);
    mat4 rotate_z(float rad);

    mat4 ortho(float left, float right, float bottom, float top, float near, float far);
    mat4 perspective(float aspectRatio, float fov, float near, float far);

    mat4 rotate(float rad, const vec3 &axis);
    mat4 rotate(const quaternion &q);

    mat4 directional_project_x(const vec3& l, float x);
    mat4 directional_project_y(const vec3& l, float y);
    mat4 directional_project_z(const vec3& l, float z);

    mat4 point_project_x(const vec3& l, float x);
    mat4 point_project_y(const vec3& l, float y);
    mat4 point_project_z(const vec3& l, float z);
}

#endif//INVICULUM_VML_TRANSFORM_HPP