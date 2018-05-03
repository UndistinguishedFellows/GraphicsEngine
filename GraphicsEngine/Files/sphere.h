#ifndef SPHERE_H
#define SPHERE_H

#include "Files/ThirdParty/glm/glm.hpp"
#include "Files/ThirdParty/glm/gtc/matrix_transform.hpp"

class Sphere {
public:
	Sphere(
		const glm::vec3 &c,
		const float &r,
		const glm::vec3 &sc,
		const bool &refl = false,
		const float &transp = 0.0f,
		const float &refrInd = 0.0f,
		const float &ef = 0.0f,
		const glm::vec3 &lc = glm::vec3(0.0f, 0.0f, 0.0f)) :
		center(c), radius(r), radius2(r * r), surfaceColor(sc), lightColor(lc),
		refractionIndex(refrInd), reflectivity(refl), transparency(transp), emission(ef)
	{ 
	}

	~Sphere() {}

	bool intersect(const glm::vec3 &rayorig, const glm::vec3 &raydir, float &t0, float &t1) const
	{
		glm::vec3 l = center - rayorig;
		float tca = glm::dot(l, raydir);
		if (tca < 0) return false;
		float d2 = glm::dot(l, l) - tca * tca;
		if (d2 > radius2) return false;
		float thc = sqrt(radius2 - d2);
		t0 = tca - thc;
		t1 = tca + thc;

		return true;
	}

	glm::vec3 getCenter() const { return center; }
	glm::vec3 getSurfaceColor() const { return surfaceColor; }
	glm::vec3 getLightColor() const { return lightColor; }
	float getRefractionIndex() const { return refractionIndex; }
	float transparencyFactor() const { return transparency; }
	float emissionFactor() const { return emission; }
	bool isLight() const { return (emission > 0.0f); }
	bool refractsLight() const { return refractionIndex != 0; }
	bool reflectsLight() const { return reflectivity; }

private:
	glm::vec3 center; 
	float radius, radius2; 
	glm::vec3 surfaceColor; 
	glm::vec3 lightColor;
	float emission;
	bool reflectivity;
	float transparency; // in the range [0.0, 1.0]
	float refractionIndex;
};

#endif
