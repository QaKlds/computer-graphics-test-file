#version 330 core
out vec4 FragColor;
uniform vec3 uFaceColor;

void main() {
    FragColor = vec4(uFaceColor, 1.0);
}