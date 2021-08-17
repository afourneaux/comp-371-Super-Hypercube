#version 330 core

in vec4 FragPos;

uniform vec3 lightPosition;
uniform float farPlane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPosition);
    
    // map to [0;1] range by dividing by farPlane
    lightDistance = lightDistance / farPlane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}
