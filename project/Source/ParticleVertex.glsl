#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aUV;

out vec2 TexCoords;
out vec4 ParticleColour;

uniform mat4 projectionMatrix;
uniform vec3 offset;
uniform vec4 colour;

void main()
{
    float scale = 10.0f;
    TexCoords = aUV;
    ParticleColour = colour;
    gl_Position = projectionMatrix * vec4((aPos * scale) + offset, 1.0);
}
