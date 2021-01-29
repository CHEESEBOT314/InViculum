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
        vec3 Li = info.light0 - posIn;
        vec3 N = normalize(normalIn);
        vec3 L = normalize(Li);
        diff = max(dot(N, L), 0.0) / length(Li);
    }

    outColour = info.colourMult * vec4(diff, diff, diff, 1.0);
}