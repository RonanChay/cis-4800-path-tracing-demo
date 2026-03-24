#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 raw_normal;
layout(location = 2) in vec4 raw_colour;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec2 fragmentTexCoord;

void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
    fragmentTexCoord = position.xy * 0.5 + 0.5;
}

#shader fragment
#version 330 core
in vec2 fragmentTexCoord;
out vec4 fragColor;
uniform sampler2D u_texture;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdrRadiance = texture(u_texture, fragmentTexCoord).rgb;
    vec3 tonemapped = ACESFilm(hdrRadiance * 0.6);
    vec3 gammaCorrectedColor = pow(tonemapped, vec3(1.0 / 2.2));
    fragColor = vec4(gammaCorrectedColor, 1.0);
}