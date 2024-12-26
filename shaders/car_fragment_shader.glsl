#version 330 core
out vec4 FragColor;

uniform vec3 carColor;

void main()
{
    FragColor = vec4(carColor, 1.0);
}