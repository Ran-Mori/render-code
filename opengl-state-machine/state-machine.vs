#version 330 core

layout (location = 0) in vec2 aPosition;

uniform vec2 uOffset;
uniform float uScale;

void main()
{
    vec2 position = aPosition * uScale + uOffset;
    gl_Position = vec4(position, 0.0, 1.0);
}
