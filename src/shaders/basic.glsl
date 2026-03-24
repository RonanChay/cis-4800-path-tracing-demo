#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 raw_normal;
layout(location = 2) in vec4 raw_colour;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
void main() {
    gl_Position = u_projection * u_view * u_model * position;
}

#shader fragment
#version 330 core

out vec4 color;

uniform vec3 u_ambientCoeff;
uniform vec3 u_diffuseCoeff;
uniform float u_materialShininess;

void main() {
    // Discard u_diffuseCoeff and u_materialShininess as not needed
    vec3 _ = u_diffuseCoeff;
    float __ = u_materialShininess;
    // u_ambientCoeff: RGB for colour
    color = vec4(u_ambientCoeff, 1.0f);
}

