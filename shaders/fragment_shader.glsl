#version 330 core
out vec4 FragColor;

in vec3 FragPos;   // Position of the fragment
in vec2 TexCoords; // Texture coordinates
in vec3 Normal;    // Normal of the fragment

uniform sampler2D texture1;
uniform vec3 lightPos;   // Position of the light
uniform vec3 viewPos;    // Camera position
uniform vec3 lightColor; // Color of the light

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine all lighting components
    vec3 result = (ambient + diffuse + specular) * texture(texture1, TexCoords).rgb;
    FragColor = vec4(result, 1.0);
}
