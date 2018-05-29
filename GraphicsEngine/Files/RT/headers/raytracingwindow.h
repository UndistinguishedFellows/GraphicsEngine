#include <QWidget>
#include "ui_raytracingwindow.h"

#include "Files/ThirdParty/glm/glm.hpp"
#include "Files/definitions.h"
#include "Files/sphere.h"
#include "AbstractWindow.h"

class MainWindow;

typedef glm::vec3 Color;

struct Ray
{
	Ray() : m_origin(glm::vec3(0.f)), m_direction(glm::vec3(0.f))
	{}

	Ray(glm::vec3& origin, glm::vec3& direction) : m_origin(origin), m_direction(direction)
	{}

	glm::vec3 m_origin, m_direction;
};

struct HitInfo
{
	HitInfo() : m_distanceHit(0.f), m_positionHit(glm::vec3(0.f)), m_normalHit(glm::vec3(0.f)), m_colorHit(glm::vec3(0.f)), m_isInside(m_isInside)
	{}

	HitInfo(float& distanceHit, glm::vec3& positionHit, glm::vec3& normalHit, glm::vec3& colorHit, bool& isInside) : m_distanceHit(distanceHit), m_positionHit(positionHit), m_normalHit(normalHit), m_colorHit(colorHit), m_isInside(isInside)
	{}

	float m_distanceHit;
	glm::vec3 m_positionHit, m_normalHit, m_colorHit;
	bool m_isInside;
};

class RayTracingWindow : public AbstractWindow
{
	Q_OBJECT

public:
	RayTracingWindow(MainWindow* mw);
	~RayTracingWindow();

private slots:
	void DockUndock() override;
	void RaytraceScene();
	void MaxRayDepthChanged(int value);
	void Cancel();
	void ShowRenderProgressChanged(bool value);

signals:
	void RenderingProgress(int);
	
private:
	void InitGUI();
	void RenderIntoTexture(glm::vec3* image, int width, int height);
	void ClearImage(glm::vec3* image, int width, int height);


	// Ray Tracing
	Color TraceRay(Ray& ray, const int &depth)const;

	void Render();
	
	bool Intersection(const Sphere &sphere, const Ray& ray, HitInfo& hitInfo)const;

	Color& BlendReflRefrColors(const Sphere* sphere, const glm::vec3 &rayDir, const glm::vec3 &normalHit, const Color &reflColor, const Color &refrColor)const;

	Ray& CalcReflectionRay(const Ray& ray, const HitInfo& hitInfo)const;
	Ray& CalcRefractionRay(const Ray& ray, const HitInfo& hitInfo, const Sphere* sphere)const;
	Color& CalcDiffuseColor(HitInfo& hitInfo, Sphere* sphere)const;


private:
	/* Attributes */
	// Screen
	int m_width;
	int m_height;
	glm::vec3 m_backgroundColor;

	bool m_renderProgress = false;

	int m_maxRayDepth;

	bool m_cancel = false;
	
	Ui::RayTracingWindow m_ui;

	std::vector<Sphere> m_spheres;

	float m_epsilonFactor;
};
