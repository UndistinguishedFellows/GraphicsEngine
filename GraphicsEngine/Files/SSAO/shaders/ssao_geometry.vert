#version 330 core
in vec3 vertex;
in vec3 normal;
in vec3 matamb;
in vec3 matdiff;
in vec3 matspec;
in float matshin;

out vec3 vertexOCS;
out vec3 normalOCS;
out vec3 fmatamb;
out vec3 fmatdiff;
out vec3 fmatspec;
out float fmatshin;

uniform mat4 projTransform;
uniform mat4 viewTransform;
uniform mat4 sceneTransform;

void main()
{
    fmatdiff = matdiff;
    fmatamb = matamb;
    fmatspec = matspec;
    fmatshin = matshin;

    vertexOCS = (viewTransform * sceneTransform * vec4(vertex, 1.0)).xyz; 
    
    mat3 normalMatrix = transpose(inverse(mat3(viewTransform * sceneTransform)));
    normalOCS = normalize(normalMatrix * normal);
    
    gl_Position = projTransform * vec4(vertexOCS, 1.0);
}