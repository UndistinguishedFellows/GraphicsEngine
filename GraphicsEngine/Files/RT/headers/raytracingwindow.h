#include <QWidget>
#include "ui_raytracingwindow.h"

#include "../glm/glm.hpp"
#include "../headers/definitions.h"
#include "../headers/sphere.h"

class MainWindow;


class RayTracingWindow : public QWidget
{
	Q_OBJECT

public:
	RayTracingWindow(MainWindow* mw);
	~RayTracingWindow();

private slots:
	void dockUndock();
	void raytraceScene();

signals:
	void renderingProgress(int);
	
private:
	void initGUI();


	// Ray Tracing
	glm::vec3 traceRay(
		const glm::vec3 &rayOrig,
		const glm::vec3 &rayDir,
		const std::vector<Sphere> &spheres,
		const int &depth);

	void render(const std::vector<Sphere> &spheres);
	
	bool intersection(
		const Sphere &sphere,
		const glm::vec3 &rayOrig,
		const glm::vec3 &rayDir,
		float &distHit,
		glm::vec3 &posHit,
		glm::vec3 &normalHit,
		glm::vec3 &colorHit, 
		bool &isInside);

	glm::vec3 blendReflRefrColors(
		const Sphere* sphere,
		const glm::vec3 &rayDir,
		const glm::vec3 &normalHit,
		const glm::vec3 &reflColor,
		const glm::vec3 &refrColor);

	/* Attributes */
	// Screen
	int m_width;
	int m_height;
	glm::vec3 background_color;
	
	Ui::RayTracingWindow m_ui;
	MainWindow* m_mainWindow;
};
