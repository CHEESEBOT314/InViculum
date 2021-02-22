#ifndef INVICULUM_VML_VEC2_HPP
#define INVICULUM_VML_VEC2_HPP

/**
 * This is a header file, please see source file in src/main instead
 */
namespace vml {
    struct alignas(8) vec2 {
        float data[2];
        vec2();
        vec2(float x, float y);
        vec2(const vec2& v);

        vec2& operator=(const vec2& v);
        vec2& operator+=(const vec2& v);
        vec2& operator-=(const vec2& v);
        vec2& operator*=(float s);
        vec2& operator/=(float s);

        float& operator[](int i);
        float const& operator[](int i) const;

        float magnitude() const;
    };

    vec2 operator+(const vec2& v);
    vec2 operator-(const vec2& v);

    vec2 operator+(const vec2& v0, const vec2& v1);
    vec2 operator-(const vec2& v0, const vec2& v1);

    vec2 operator*(const vec2& v, float s);
    vec2 operator*(float s, const vec2& v);
    vec2 operator/(const vec2& v, float s);
}

#endif//INVICULUM_VML_VEC2_HPP