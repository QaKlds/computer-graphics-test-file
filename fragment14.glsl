#version 330 core

in vec3 fragColor;
out vec4 FragColor;

uniform bool uUseVertexColor;
uniform vec3 uColor;

void main() {
    if (uUseVertexColor) {
        FragColor = vec4(fragColor, 1.0f);
    } else {
        FragColor = vec4(uColor, 1.0f);
    }
}