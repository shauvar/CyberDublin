#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 carColor;
uniform bool brakingLights;
uniform float headlightIntensity;
uniform bool movingForward;

void main() {
    // Basic lighting parameters
    vec3 lightPos = vec3(5.0, 10.0, 5.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float ambientStrength = 0.3;
    vec3 viewPos = vec3(0.0, 5.0, 10.0);

    // Ambient lighting
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

    // Combine lighting with car color
    vec3 result = (ambient + diffuse + specular) * carColor;

    // Add headlights (front of car)
    if (FragPos.z < -0.5) {  // Front of car
        float headlightGlow = headlightIntensity * 0.5;
        result += vec3(1.0, 1.0, 0.8) * headlightGlow;
    }

    // Add brake lights (back of car)
    if (FragPos.z > 0.5 && brakingLights) {  // Back of car
        result += vec3(0.8, 0.0, 0.0) * 0.5;  // Red brake lights
    }

    FragColor = vec4(result, 1.0);
}