#include "Files/RT/headers/raytracingwindow.h"
#include "Files/mainwindow.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>

#include <iostream>
#include <fstream>


RayTracingWindow::RayTracingWindow(MainWindow* mw) : AbstractWindow(mw)
{
	m_ui.setupUi(this);

	std::cout << "=== Raytracing window" << std::endl;

	initGUI();

	m_width = m_ui.qRayTracingView->width() - 2;
	m_height = m_ui.qRayTracingView->height() - 2;
	mBackgroundColor = glm::vec3(0.2f);

	m_maxRayDepth = MAX_RAY_DEPTH;

	connect(m_ui.qUndockButton, SIGNAL(clicked()), this, SLOT(dockUndock()));
	connect(m_ui.qRenderButton, SIGNAL(clicked()), this, SLOT(raytraceScene()));
	connect(this, SIGNAL(renderingProgress(int)), m_ui.qProgressBar, SLOT(setValue(int)));
	connect(m_ui.maxRayDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(maxRayDepthChanged(int)));
}

RayTracingWindow::~RayTracingWindow(){ }

void RayTracingWindow::dockUndock()
{
	if (parent()) 
	{
		setParent(0);
		setAttribute(Qt::WA_DeleteOnClose);
		move(QApplication::desktop()->width() / 2 - width() / 2,
			QApplication::desktop()->height() / 2 - height() / 2);
		m_ui.qUndockButton->setText(tr("Dock"));
		show();
	}
	else 
	{
		if (!mainWindow->centralWidget()) 
		{
			if (mainWindow->isVisible()) 
			{
				setAttribute(Qt::WA_DeleteOnClose, false);
				m_ui.qUndockButton->setText(tr("Undock"));
				mainWindow->setCentralWidget(this);
				show();
			}
			else 
			{
				QMessageBox::information(0, tr("Cannot dock"), tr("Main window already closed"));
			}
		}
		else 
		{
			QMessageBox::information(0, tr("Cannot dock"), tr("Main window already occupied"));
		}
	}
}

void RayTracingWindow::initGUI()
{
	QGraphicsScene *scene = new QGraphicsScene();
	scene->clear();
	scene->addText("(Empty)");

	m_ui.qRayTracingView->resetMatrix();
	m_ui.qRayTracingView->setScene(scene);
	m_ui.qRayTracingView->show();
}

void RayTracingWindow::renderScene(glm::vec3* image, int width, int height)
{
	QImage img(width, height, QImage::Format_ARGB32);

	int id_img = 0;
	for (unsigned j = 0; j < height; ++j) {
		for (unsigned i = 0; i < width; ++i) {
			QColor col(MIN(255, image[id_img].x * 255),
				MIN(255, image[id_img].y * 255),
				MIN(255, image[id_img].z * 255));
			img.setPixelColor(i, j, col);
			id_img++;
		}
	}

	QGraphicsScene* imgView = new QGraphicsScene();
	imgView->addPixmap(QPixmap::fromImage(img));
	m_ui.qRayTracingView->setScene(imgView);
	m_ui.qRayTracingView->show();
}

glm::vec3 RayTracingWindow::traceRay(const Ray& ray, const std::vector<Sphere> &spheres, const int &depth)
{
	Sphere* sphere = nullptr;
	float minDist = INFINITY;
	HitInfo closestHitInfo;

	std::vector<const Sphere*> lights;

	for(int i = 0; i < spheres.size(); ++i)
	{
		const Sphere* it = &spheres[i];

		if(!it->isLight())
		{
			if (intersection(*it, ray, closestHitInfo))
			{
				// Calc the distance
				if (closestHitInfo.m_distanceHit < minDist)
				{
					// Assign the closer sphere
					sphere = (Sphere*)it;
					minDist = closestHitInfo.m_distanceHit;
				}
			}
		}
		else
		{
			lights.push_back(it);
		}
	}

	if(!sphere)
	{
		// If no collision take the background color
		return mBackgroundColor;
	}

	// If there's a collision get the shadow value and calc the resultant color

	float shadow = 1.f;

	for(int l = 0; l < lights.size(); ++l)
	{
		const Sphere* light = lights[l];
		glm::vec3 shadowRayDir = light->getCenter() - closestHitInfo.m_positionHit;
		shadowRayDir = glm::normalize(shadowRayDir);
		glm::vec3 shadowRayOrigin = closestHitInfo.m_positionHit + shadowRayDir * glm::vec3(0.01f);

		for (int i = 0; i < spheres.size(); ++i)
		{
			if (!spheres[i].isLight())
			{				
				Ray shadowRay(shadowRayOrigin, shadowRayDir);
				HitInfo hitInfo;

				if (intersection(spheres[i], shadowRay, hitInfo) && !hitInfo.m_isInside)
				{
					// Shadow
					shadow -= 0.3f;
				}
			}
		}
	}

	if (shadow < 0.f) shadow = 0.f;

	return sphere->getSurfaceColor() * glm::vec3(shadow);
}

void RayTracingWindow::render(const std::vector<Sphere> &spheres)
{
	m_width = m_ui.qRayTracingView->width() - 2;
	m_height = m_ui.qRayTracingView->height() - 2;
	
	glm::vec3 *image = new glm::vec3[m_width * m_height], *pixel = image;
	for(int x = 0; x <m_width; ++x)
	{
		for(int y = 0; y < m_height; ++y, ++pixel)
		{
			*pixel = mBackgroundColor;
		}
	}

	pixel = image;

	float invWidth = 1 / float(m_width), invHeight = 1 / float(m_height);
	float fov = 30, aspectratio = m_width / float(m_height);
	float angle = tan(PI * 0.5 * fov / 180.);
	
	int progress = 0;
	int numPixels = m_width * m_height;
	
	int nextPercentageToRender = 10, incrementPercentage = 10;
	// Trace rays
	for (unsigned y = 0; y < m_height; ++y) 
	{
		for (unsigned x = 0; x < m_width; ++x, ++pixel) 
		{
			float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
			float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
			glm::vec3 rayDir(xx, yy, -1);
			rayDir = glm::normalize(rayDir);
			glm::vec3 rayOrig(0.0f, 0.0f, 0.0f);

			Ray ray(rayOrig, rayDir);
			*pixel = traceRay(ray, spheres, 0);
			
			progress++;

			int percentage = (int)(float)progress / (float)numPixels * 100;
			emit renderingProgress(percentage);

			if(m_renderProgress && percentage >= nextPercentageToRender)
			{
				// Each 10% render the image
				renderScene(image, m_width, m_height);
				nextPercentageToRender += incrementPercentage;
			}
		}
	}

	if(!m_renderProgress) renderScene(image, m_width, m_height);

	delete[] image;
}

void RayTracingWindow::raytraceScene() 
{
	std::vector<Sphere> spheres;

	// Lights
	spheres.push_back(Sphere(glm::vec3(10.0f, 20.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));
	spheres.push_back(Sphere(glm::vec3(-10.0f, 20.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));
	spheres.push_back(Sphere(glm::vec3(0.0f, 10.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));

	// Spheres of the scene
	spheres.push_back(Sphere(glm::vec3(0.0, -10004, -30), 10000, glm::vec3(0.0f, 0.2f, 0.5f), false, 0.0, 0.0));
	spheres.push_back(Sphere(glm::vec3(0.0f, 0.0f, -20.0f), 2, glm::vec3(1.0f, 1.0f, 1.0f), true, 0.9f, 1.1f));
	spheres.push_back(Sphere(glm::vec3(4.0f, 0.0f, -32.5f), 4, glm::vec3(0.0f, 0.5f, 0.0f), true, 0.0f, 0.0f));
	spheres.push_back(Sphere(glm::vec3(-5.0f, 0.0f, -35.0f), 3, glm::vec3(0.5f, 0.5f, 0.5f), true, 0.0f, 0.0f));
	spheres.push_back(Sphere(glm::vec3(-4.5f, -1.0f, -19.0f), 1.5f, glm::vec3(0.5f, 0.1f, 0.0f), true, 0.0f, 0.0f));

	//TODO: UNCOMMENT THE NEXT LINE TO RENDER THE SCENE
	render(spheres);
}

void RayTracingWindow::maxRayDepthChanged(int value)
{
	m_maxRayDepth = value;
}

bool RayTracingWindow::intersection(const Sphere &sphere, const Ray& ray, HitInfo& hitInfo)
{

	float inter0 = INFINITY;
	float inter1 = INFINITY;

	if (sphere.intersect(ray.m_origin, ray.m_direction, inter0, inter1)) 
	{
		if (inter0 < 0)
			inter0 = inter1;

		hitInfo.m_distanceHit = inter0;
		hitInfo.m_positionHit = ray.m_origin + ray.m_direction * inter0;
		hitInfo.m_normalHit = hitInfo.m_positionHit - sphere.getCenter();
		hitInfo.m_normalHit = glm::normalize(hitInfo.m_normalHit);

		// If the normal and the view direction are not opposite to each other
		// reverse the normal direction. That also means we are inside the sphere so set
		// the inside bool to true.
		hitInfo.m_isInside = false;
		float dotProd = glm::dot(ray.m_direction, hitInfo.m_normalHit);
		
		if (dotProd > 0) {
			hitInfo.m_normalHit = -hitInfo.m_normalHit;
			hitInfo.m_isInside = true;
		}

		hitInfo.m_colorHit = sphere.getSurfaceColor();

		return true;
	}
	else 
	{
		return false;
	}
}

glm::vec3 RayTracingWindow::blendReflRefrColors(const Sphere* sphere, const glm::vec3 &raydir, const glm::vec3 &normalHit, const glm::vec3 &reflColor, const glm::vec3 &refrColor) 
{
	float facingRatio = -glm::dot(raydir, normalHit);
	float fresnel = 0.5f + pow(1 - facingRatio, 3) * 0.5;

	glm::vec3 blendedColor = (reflColor * fresnel + refrColor * (1 - fresnel) * sphere->transparencyFactor())*sphere->getSurfaceColor();
	return blendedColor;
}