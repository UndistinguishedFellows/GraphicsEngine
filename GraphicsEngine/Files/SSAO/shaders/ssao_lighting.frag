#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPhong;
uniform sampler2D ssao;
uniform sampler2D normal;
uniform sampler2D gPosition;

uniform int renderType;

void main()
{             
    // retrieve data from gbuffer
    vec3 Phong = texture(gPhong, TexCoords).rgb;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    
    if(renderType == 0)
        FragColor = vec4(AmbientOcclusion * Phong, 1.0);
    else if(renderType == 1)
        FragColor = texture(gPosition, TexCoords);
    else if(renderType == 2)
        FragColor = texture(normal, TexCoords);
    else if(renderType == 3)
        FragColor = vec4(vec3(AmbientOcclusion), 1.0);
    else if(renderType == 4)
        FragColor = vec4(Phong, 1.0);
}