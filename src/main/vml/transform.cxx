#include "vml/transform.hxx"

const float PI = 3.1415926535897932384f;
#include <cmath>

/**
 * vml - VML (Vector Maths Library) namespace stores all linear algebra methods and types
 * the following file defines the useful transforms
 */
namespace vml {
    // Scale the object by s in all directions
    mat3 scale(float s) {
        return mat3(
                 s,    0.0f, 0.0f,
            0.0f,      s,    0.0f,
            0.0f, 0.0f,      s   );
    }
    // Scale the object by v in its respective directions
    mat3 scale(const vec3 &v) {
        return mat3(
            v[0], 0.0f, 0.0f,
            0.0f, v[1], 0.0f,
            0.0f, 0.0f, v[2]);
    }
    // Translate the object by the vector v
    mat4 translate(const vec3 &v) {
        return mat4(
            1.0f,  0.0f,  0.0f,  0.0f,
            0.0f,  1.0f,  0.0f,  0.0f,
            0.0f,  0.0f,  1.0f,  0.0f,
            v[0],  v[1],  v[2],  1.0f);
    }
    // Translate the transformed object by the vector v, commonly used with the function scale
    mat4 translate(const mat3 &m, const vec3 &v) {
        mat4 out = mat4::extend(m);
        out[3][0] = v[0];
        out[3][1] = v[1];
        out[3][2] = v[2];
        return out;
    }
    // Rotate the object about the x axis by rad amount
    mat4 rotate_x(float rad) {
        float c = std::cos(rad);
        float s = std::sin(rad);
        return mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, c, s, 0.0f,
                0.0f, -s, c, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
    }
    // Rotate the object about the y axis by rad amount
    mat4 rotate_y(float rad) {
        float c = std::cos(rad);
        float s = std::sin(rad);
        return mat4(
                c, 0.0f, -s, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                s, 0.0f, c, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
    }
    // Rotate the object about the z axis by rad amount
    mat4 rotate_z(float rad) {
        float c = std::cos(rad);
        float s = std::sin(rad);
        return mat4(
            c, s, 0.0f, 0.0f,
            -s, c, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }
    // Creates an orthographic matrix used in most 2D games, not used in this application
    mat4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        mat4 out;
        out[0][0] = 2.0f / (right - left);
        out[3][0] = (left + right) / (left - right);
        out[1][1] = 2.0f / (bottom - top);
        out[3][1] = (top + bottom) / (top - bottom);
        out[2][2] = 1.0f / (far - near);
        out[3][2] = near / (near - far);
        out[3][3] = 1.0f;
        return out;
    }
    // Creates a perspective matrix used in most 3D games, used in this application, provides parallax effect
    mat4 perspective(float aspectRatio, float fovPiRad, float near, float far) {
        mat4 out;
        float s = 1.0f / std::tan(0.5f * PI * fovPiRad);
        out[0][0] = s / aspectRatio;
        out[1][1] = -s;
        out[2][2] = far / (near - far);
        out[2][3] = -1.0f;
        out[3][2] = near * far / (near - far);
        return out;
    }
    // Creates a perspective matrix (different method)
    mat4 perspective_p(float aspectRatio, float width, float near, float far) {
        mat4 out;
        float s = (2.0f * near) / width;
        out[0][0] = s / aspectRatio;
        out[1][1] = -s;
        out[2][2] = far / (near - far);
        out[3][2] = near * far / (near - far);
        out[2][3] = -1.0f;
        return out;
    }
    // Rotate the object about the given axis by rad amount
    mat4 rotate(float rad, const vec3 &axis) {
        vec3 unit = axis / axis.magnitude();
        float s = std::sin(rad / 2);
        return rotate(quaternion(std::cos(rad / 2), s * unit[0], s * unit[1], s * unit[2]));
    }
    // Rotate the object using the given quaternion rotation, not used but present for completion
    mat4 rotate(const quaternion &q) {
        return mat4(
                1.0f-2.0f*(q[2]*q[2]+q[3]*q[3]), 2.0f*(q[1]*q[2]+q[3]*q[0]),      2.0f*(q[1]*q[3]-q[2]*q[0]),      0.0f,
                2.0f*(q[1]*q[2]-q[3]*q[0]),      1.0f-2.0f*(q[1]*q[1]+q[3]*q[3]), 2.0f*(q[2]*q[3]+q[1]*q[0]),      0.0f,
                2.0f*(q[1]*q[3]+q[2]*q[0]),      2.0f*(q[2]*q[3]-q[1]*q[0]),      1.0f-2.0f*(q[1]*q[1]+q[2]*q[2]), 0.0f,
                0.0f,                            0.0f,                            0.0f,                            1.0f);
    }

    // Creates the directional projection matrix onto the given x plane using the light direction, see report for derivation
    mat4 directional_project_x(const vec3& l, float x) {
        return mat4(0.0F, -l[1] / l[0], -l[2] / l[0], 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, x, x * l[1] / l[0], x * l[2] / l[0], 1.0F);
    }
    // Creates the directional projection matrix onto the given y plane using the light direction, see report for derivation
    mat4 directional_project_y(const vec3& l, float y) {
        return mat4(1.0F, 0.0F, 0.0F, 0.0F, -l[0] / l[1], 0.0F, -l[2] / l[1], 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, y * l[0] / l[1], y, y * l[2] / l[1], 1.0F);
    }
    // Creates the directional projection matrix onto the given z plane using the light direction, see report for derivation
    mat4 directional_project_z(const vec3& l, float z) {
        return mat4(1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, -l[0] / l[2], -l[1] / l[2], 0.0F, 0.0F, z * l[0] / l[2], z * l[1] / l[2], z, 1.0F);
    }

    // Creates the point projection matrix onto the given x plane using the light direction, see report for derivation
    mat4 point_project_x(const vec3& l, float x) {
        return mat4(x, l[1], l[2], 1.0F, 0.0F, x - l[0], 0.0F, 0.0F, 0.0F, 0.0F, x - l[0], 0.0F, -x * l[0], -x * l[1], -x * l[2], -l[0]);
    }
    // Creates the point projection matrix onto the given y plane using the light direction, see report for derivation
    mat4 point_project_y(const vec3& l, float y) {
        return mat4(y - l[1], 0.0F, 0.0F, 0.0F, l[0], y, l[2], 1.0F, 0.0F, 0.0F, y - l[1], 0.0F, -y * l[0], -y * l[1], -y * l[2], -l[1]);
    }
    // Creates the point projection matrix onto the given z plane using the light direction, see report for derivation
    mat4 point_project_z(const vec3& l, float z) {
        return mat4(z - l[2], 0.0F, 0.0F, 0.0F, 0.0F, z - l[2], 0.0F, 0.0F, l[0], l[1], z, 1.0F, -z * l[0], -z * l[1], -z * l[2], -l[2]);
    }
}