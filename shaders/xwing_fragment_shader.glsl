#version 330 core

out vec4 FragColor;

uniform vec3 color; // Color of the ship

void main()
{
    FragColor = vec4(color, 1.0); // Simple solid color for now
}
