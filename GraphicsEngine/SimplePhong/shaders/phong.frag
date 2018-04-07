#version 330 core

in vec4 vertexOCS;
in vec3 normalOCS;
in vec4 vertexColor;

uniform vec3 lightPos;
uniform vec3 lightCol;
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float matShininess;

out vec4 FragColor;

vec3 ambientLight = vec3(0.2, 0.2, 0.2);

vec3 lambert(vec3 normOCS, vec3 L)
{
	vec3 ret = ambientLight * matAmbient;

	if(dot(L, normOCS) > 0)
		ret += lightCol * matDiffuse * dot(L, normOCS);

		return ret;
}

vec3 phong(vec3 normOCS, vec3 L, vec4 vertOCS)
{
	// Assume vectors are normalized

	vec3 ret = lambert(normOCS, L);

	if(dot(normOCS, L) < 0)
		return ret;

	vec3 R = reflect(-L, normOCS);
	vec3 V = normalize(-vertOCS.xyz);

	 if ((dot(R, V) < 0) || (matShininess == 0))
		return ret;

	float shin = pow(max(0.0, dot(R, V)), matShininess);
	return (ret + matSpecular * lightCol * shin);
}

void main()
{
	vec3 L = normalize(lightPos - vec3(vertexOCS).xyz);
    FragColor = vec4(phong(normalize(normalOCS), L, vertexOCS), 1);
}
