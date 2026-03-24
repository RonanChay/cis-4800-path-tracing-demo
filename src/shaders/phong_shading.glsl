#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 raw_normal;
layout(location = 2) in vec4 raw_colour;

out vec4 frag_pos;
out vec4 vertex_normal;
out vec4 vertex_colour;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
void main() {
    gl_Position = u_projection * u_view * u_model * position;
    frag_pos = u_model * position;
    vertex_normal = u_model * raw_normal;
    vertex_colour = raw_colour;
}

#shader fragment
#version 330 core
in vec4 frag_pos;
in vec4 vertex_normal;
in vec4 vertex_colour;

out vec4 color;

struct PointLight {    
    vec3 position;
    vec3 point_intensity;

    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;  
};
uniform PointLight u_pointLights[10];

uniform vec3 u_viewpos;
uniform vec3 u_ambientIntensity;
uniform int u_num_pointLights;
uniform vec3 u_ambientCoeff;
uniform vec3 u_diffuseCoeff;
uniform vec3 u_specularCoeff;
uniform vec3 u_specularMetalColour;
uniform float u_materialShininess;
uniform vec4 u_color;
uniform float u_alphaColour;
void main() {
    vec4 surfaceNormal = normalize(vertex_normal);
    vec4 viewDirection = normalize(vec4(u_viewpos, 1.0) - frag_pos);

    color = vertex_colour * vec4(u_ambientCoeff, 1.0) * vec4(u_ambientIntensity, 1.0);

    for (int i = 0; i < u_num_pointLights; i++) {
        PointLight pl = u_pointLights[i];

        vec4 lightDirection = normalize(vec4(pl.position, 1.0) - frag_pos);
        vec4 reflectionDirection = -lightDirection + 2.0*dot(surfaceNormal,lightDirection)*surfaceNormal;
        float distanceToLight = distance(frag_pos, vec4(pl.position, 1.0));
        float attenuation = 1.0 / (
            pl.constant_attenuation 
            + pl.linear_attenuation * distanceToLight 
            + pl.quadratic_attenuation * pow(distanceToLight, 2)
        );
        attenuation = min(attenuation, 1);
        color += (
            vec4(attenuation) * vec4(pl.point_intensity, 1.0) * (
                vertex_colour * vec4(u_diffuseCoeff, 1.0) * max(dot(lightDirection, surfaceNormal), 0.0)
              + vec4(u_specularMetalColour,1.0) * vec4(u_specularCoeff,1.0) * pow(max(dot(viewDirection,reflectionDirection),0.0),u_materialShininess)
            )
        );
    }
    color.w = u_alphaColour;
}

