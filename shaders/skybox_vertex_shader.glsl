#version 330 core
layout (location = 0) in vec3 aPos;

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Convert to clip space while preserving w-component for depth testing
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Force depth to be maximum
    
    // Calculate texture coordinates from vertex position
    TexCoords = vec2(aPos.x + 0.5, aPos.y + 0.5);
}