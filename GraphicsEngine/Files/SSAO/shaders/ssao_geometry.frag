#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPhong;

in vec3 vertexOCS;
in vec3 normalOCS;

in vec3 fmatamb;
in vec3 fmatdiff;
in vec3 fmatspec;
in float fmatshin;

uniform vec3 lightPos;
uniform vec3 lightCol;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vertexOCS;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(normalOCS);

    // Phong calculation
    vec3 L = normalize(lightPos - vertexOCS);
    vec3 N = normalize(normalOCS);

    float dotNL = max(dot(N,L), 0.0);

    vec3 V = normalize(-vertexOCS);
    vec3 R = reflect(-L, N);
    float dotRVs = pow(max(dot(R,V), 0.0), fmatshin);
  
    vec3 ambient = lightCol.x * fmatamb;
    vec3 diffuse = lightCol.y * fmatdiff * dotNL;
    vec3 specular;
  
    if(dot(R,V) < 0 || fmatshin == 0 )
        specular = vec3(0.0, 0.0, 0.0);
    else
        specular = lightCol.z * fmatspec * dotRVs;

    gPhong = diffuse + specular + ambient;
}