#version 450
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Info {
    mat4 p;
    mat4 v;
    mat4 m;
    mat4 colourMult;
// Dir Light
    vec3 lightDir;
// Single Light
    vec3 light0;
// Multi Light
    vec3 light1;

    bool isShadow;
} info;

layout(location = 0) in vec2 posIn;
layout(location = 1) in vec2 uvIn;

layout(location = 0) out vec2 uvOut;
layout(location = 1) out vec3 normalOut;
layout(location = 2) out vec3 posOut;

void main() {
    uvOut = uvIn;

    vec4 pos = info.m * vec4(posIn, 0.0, 1.0);
    pos /= pos.w;
    posOut = pos.xyz;

    mat4 mv = info.v * info.m;
    normalOut = mat3(mv) * vec3(0.0, 0.0, 1.0);
    gl_Position = info.p * info.v * pos;
}