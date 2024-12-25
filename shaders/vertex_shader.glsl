#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aOffset;
layout (location = 4) in vec3 aScale;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec3 scaledPos = aPos * aScale;
    vec3 worldPos = scaledPos + aOffset;
    
    FragPos = worldPos;
    TexCoords = aTexCoords;
    Normal = normalize(aNormal * mat3(1.0));  // Simplified normal transformation
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
}