#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D skyboxTexture;

void main()
{    
    vec4 texColor = texture(skyboxTexture, TexCoords);
    FragColor = texColor;
}