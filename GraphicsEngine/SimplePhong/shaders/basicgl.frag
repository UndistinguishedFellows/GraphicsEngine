#version 330 core

in vec4 vertexOCS;
in vec3 normalOCS;
in vec4 vertexColor;

out vec4 FragColor;

void main()
{
    FragColor = vertexColor;
}
