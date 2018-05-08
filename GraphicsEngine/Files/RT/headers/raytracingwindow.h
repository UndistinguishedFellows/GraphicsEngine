#include <QWidget>
#include "ui_raytracingwindow.h"

#include "Files/ThirdParty/glm/glm.hpp"
#include "Files/definitions.h"
#include "Files/sphere.h"
#include "AbstractWindow.h"

class MainWindow;

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
	void dockUndock() override;
	void raytraceScene();
	void maxRayDepthChanged(int value);

signals:
	void renderingProgress(int);
	
private:
	void initGUI();
	void renderScene(glm::vec3* image, int width, int height);


	// Ray Tracing
	glm::vec3 traceRay(const Ray& ray, const std::vector<Sphere> &spheres, const int &depth);

	void render(const std::vector<Sphere> &spheres);
	
	bool intersection(const Sphere &sphere, const Ray& ray, HitInfo& hitInfo);

	glm::vec3 blendReflRefrColors(const Sphere* sphere, const glm::vec3 &rayDir, const glm::vec3 &normalHit, const glm::vec3 &reflColor, const glm::vec3 &refrColor);

	/* Attributes */
	// Screen
	int m_width;
	int m_height;
	glm::vec3 mBackgroundColor;

	bool m_renderProgress = false;

	int m_maxRayDepth;
	
	Ui::RayTracingWindow m_ui;
};
