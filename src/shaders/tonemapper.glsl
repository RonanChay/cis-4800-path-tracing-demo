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

void main() {
    vec3 colour = texture(u_texture, fragmentTexCoord).rgb;
    fragColor = vec4(colour, 1.0);
}