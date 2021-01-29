#version 450
#pragma shader_stage(fragment)
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

layout(location = 0) in vec2 uvIn;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec3 posIn;

layout(location = 0) out vec4 outColour;

void main() {

    float diff = 1.0;
    if (!info.isShadow) {
        vec3 Li0 = info.light0 - posIn;
        vec3 Li1 = info.light1 - posIn;
        vec3 N = normalize(normalIn);
        vec3 L0 = normalize(Li0);
        vec3 L1 = normalize(Li1);
        diff = max(dot(N, L0), 0.0) / length(Li0) + max(dot(N, L1), 0.0) / length(Li1);
    }

    outColour = info.colourMult * vec4(diff, diff, diff, 1.0);
}