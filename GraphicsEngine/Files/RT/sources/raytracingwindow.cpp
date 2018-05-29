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

	InitGUI();

	m_width = m_ui.qRayTracingView->width() - 2;
	m_height = m_ui.qRayTracingView->height() - 2;
	m_backgroundColor = glm::vec3(0.9f);

	m_epsilonFactor = 1e-4f;

	m_maxRayDepth = MAX_RAY_DEPTH;

	m_ui.maxRayDepthSpinBox->setValue(m_maxRayDepth);

	connect(m_ui.qUndockButton, SIGNAL(clicked()), this, SLOT(DockUndock()));
	connect(m_ui.qRenderButton, SIGNAL(clicked()), this, SLOT(RaytraceScene()));
	connect(this, SIGNAL(RenderingProgress(int)), m_ui.qProgressBar, SLOT(setValue(int)));
	connect(m_ui.maxRayDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(MaxRayDepthChanged(int)));
	//connect(m_ui.qCancelButton, SIGNAL(clicked()), this, SLOT(Cancel()));
	connect(m_ui.qRenderProgressCheckBox, SIGNAL(clicked(bool)), this, SLOT(ShowRenderProgressChanged(bool)));
	connect(m_ui.qSaveToTexture, SIGNAL(clicked()), this, SLOT(OnSave()));
}

RayTracingWindow::~RayTracingWindow(){ }

void RayTracingWindow::DockUndock()
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

void RayTracingWindow::InitGUI()
{
	QGraphicsScene *scene = new QGraphicsScene();
	scene->clear();
	scene->addText("(Empty)");

	m_ui.qRayTracingView->resetMatrix();
	m_ui.qRayTracingView->setScene(scene);
	m_ui.qRayTracingView->show();
}

void RayTracingWindow::RenderIntoTexture(glm::vec3* image, int width, int height)
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

void RayTracingWindow::ClearImage(glm::vec3* image, int width, int height)
{
	glm::vec3* pixel = image;
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y, ++pixel)
		{
			*pixel = m_backgroundColor;
		}
	}

	RenderIntoTexture(image, m_width, m_height);
}

Color RayTracingWindow::TraceRay(Ray& ray, const int &depth)const
{
	ray.m_direction = glm::normalize(ray.m_direction);

	Sphere* sphere = nullptr;
	float minDist = INFINITY;
	HitInfo closestHitInfo;

	for(int i = 0; i < m_spheres.size(); ++i)
	{
		const Sphere* it = &m_spheres[i];

		if(!it->isLight())
		{
			if (Intersection(*it, ray, closestHitInfo))
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
	}

	if(!sphere)
	{
		// If no collision take the background color
		return m_backgroundColor;
	}

	if (sphere->isLight())
	{
		// If the closest intersection is a light return its color
		return sphere->getLightColor(); // *sphere->emissionFactor();
	}

	// ---------------------------------------

	Color colorRay(0.f);

	if((sphere->refractsLight() || sphere->reflectsLight()) && depth < m_maxRayDepth)
	{
		// Reflection
		Ray reflectRay = CalcReflectionRay(ray, closestHitInfo);

		const Color reflColor = TraceRay(reflectRay, depth + 1);


		// Refraction
		Color refrColor(0.f);

		if (sphere->refractsLight())
		{
			// Calc refraction ray
			Ray refractionRay = CalcRefractionRay(ray, closestHitInfo, sphere);

			refrColor = TraceRay(refractionRay, depth + 1);
		}

		colorRay = BlendReflRefrColors(sphere, ray.m_direction, closestHitInfo.m_normalHit, reflColor, refrColor);
	}
	else
	{
		// Diffuse object

		colorRay = CalcDiffuseColor(closestHitInfo, sphere);
	}


	return colorRay + sphere->getLightColor() * sphere->emissionFactor(); 
}

void RayTracingWindow::Render()
{
	m_width = m_ui.qRayTracingView->width() - 2;
	m_height = m_ui.qRayTracingView->height() - 2;
	
	glm::vec3 *image = new glm::vec3[m_width * m_height], *pixel = image;
	
	ClearImage(image, m_width, m_height);

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
			if(m_cancel)
			{
				m_cancel = false;
				ClearImage(image, m_width, m_height);
				return;
			}

			float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
			float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
			glm::vec3 rayDir(xx, yy, -1);
			rayDir = glm::normalize(rayDir);
			glm::vec3 rayOrig(0.0f, 0.0f, 0.0f);

			Ray ray(rayOrig, rayDir);
			*pixel = TraceRay(ray, 0);// / Color(m_maxRayDepth);
			
			progress++;

			int percentage = (int)(float)progress / (float)numPixels * 100;
			emit RenderingProgress(percentage);

			if(m_renderProgress && percentage >= nextPercentageToRender)
			{
				// Each 10% render the image
				RenderIntoTexture(image, m_width, m_height);
				nextPercentageToRender += incrementPercentage;
			}
		}
	}

	if(!m_renderProgress) RenderIntoTexture(image, m_width, m_height);

	delete[] image;
}

void RayTracingWindow::RaytraceScene() 
{
	m_spheres.clear();
	//m_lights.clear();

	// Lights
	m_spheres.push_back(Sphere(glm::vec3(10.0f, 20.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));
	m_spheres.push_back(Sphere(glm::vec3(-10.0f, 20.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));
	m_spheres.push_back(Sphere(glm::vec3(0.0f, 10.0f, 0.0f), 2, glm::vec3(0.0f, 0.0f, 0.0f), false, 0.0f, 0.0f, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f)));

	// Spheres of the scene
	m_spheres.push_back(Sphere(glm::vec3(0.0, -10004, -30), 10000, glm::vec3(0.0f, 0.2f, 0.5f), false, 0.0, 0.0));
	m_spheres.push_back(Sphere(glm::vec3(0.0f, 0.0f, -20.0f), 2, glm::vec3(1.0f, 1.0f, 1.0f), true, 0.9f, 1.1f));
	m_spheres.push_back(Sphere(glm::vec3(4.0f, 0.0f, -32.5f), 4, glm::vec3(0.0f, 0.5f, 0.0f), true, 0.0f, 0.0f));
	m_spheres.push_back(Sphere(glm::vec3(-5.0f, 0.0f, -35.0f), 3, glm::vec3(0.5f, 0.5f, 0.5f), true, 0.0f, 0.0f));
	m_spheres.push_back(Sphere(glm::vec3(-4.5f, -1.0f, -19.0f), 1.5f, glm::vec3(0.5f, 0.1f, 0.0f), true, 0.0f, 0.0f));

	Render();
}

void RayTracingWindow::MaxRayDepthChanged(int value)
{
	m_maxRayDepth = value;
}

void RayTracingWindow::Cancel()
{
	m_cancel = true;
}

void RayTracingWindow::ShowRenderProgressChanged(bool value)
{
	m_renderProgress = value;
}

bool RayTracingWindow::Intersection(const Sphere &sphere, const Ray& ray, HitInfo& hitInfo)const
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

Color& RayTracingWindow::BlendReflRefrColors(const Sphere* sphere, const glm::vec3 &raydir, const glm::vec3 &normalHit, const Color &reflColor, const Color &refrColor)const
{
	const float facingRatio = -glm::dot(raydir, normalHit);
	const float fresnel = 0.5f + pow(1 - facingRatio, 3) * 0.5;

	Color blendColor = (reflColor * fresnel + refrColor * (1 - fresnel) * sphere->transparencyFactor()) * sphere->getSurfaceColor();
	return blendColor;
}

Ray & RayTracingWindow::CalcReflectionRay(const Ray & ray, const HitInfo & hitInfo)const
{
	Ray reflection;

	reflection.m_direction = glm::normalize(glm::reflect(ray.m_direction, hitInfo.m_normalHit));

	glm::vec3 epsilon = hitInfo.m_normalHit * m_epsilonFactor;
	reflection.m_origin = hitInfo.m_positionHit + epsilon;// +(hitInfo.m_isInside ? -epsilon : epsilon);

	return reflection;
}

Ray & RayTracingWindow::CalcRefractionRay(const Ray & ray, const HitInfo & hitInfo, const Sphere * sphere)const
{
	Ray refraction;

	float index = hitInfo.m_isInside ? sphere->getRefractionIndex() : 1.f / sphere->getRefractionIndex();

	refraction.m_direction = glm::normalize(glm::refract(ray.m_direction, hitInfo.m_normalHit, index));

	glm::vec3 epsilon = hitInfo.m_normalHit * m_epsilonFactor;
	refraction.m_origin = hitInfo.m_positionHit - epsilon;// +(hitInfo.m_isInside ? -epsilon : epsilon);

	return refraction;
}

Color& RayTracingWindow::CalcDiffuseColor(HitInfo& hitInfo, Sphere* sphere)const
{
	Color diffuse(0.f);

	for (int l = 0; l < m_spheres.size(); ++l)
	{
		const Sphere* light = &m_spheres[l];

		if(light->isLight())
		{
			Ray shadowRay;
			shadowRay.m_direction = glm::normalize(light->getCenter() - hitInfo.m_positionHit);
			const glm::vec3 epsilon = hitInfo.m_normalHit * m_epsilonFactor;
			shadowRay.m_origin = hitInfo.m_positionHit + (hitInfo.m_isInside ? -epsilon : epsilon);

			float invShadow = 1.f;

			for (int i = 0; i < m_spheres.size(); ++i)
			{
				if (!m_spheres[i].isLight())
				{
					HitInfo shadowHitInfo;

					if (Intersection(m_spheres[i], shadowRay, shadowHitInfo))
					{
						invShadow = 0.f;
						break;
					}
				}
			}

			diffuse += sphere->getSurfaceColor() * invShadow * std::max(0.f, glm::dot(hitInfo.m_normalHit, shadowRay.m_direction)) * light->getLightColor() * light->emissionFactor();
		}
	}

	return diffuse;
}

void RayTracingWindow::OnSave()
{
	// Get the file path
	QString path = QFileDialog::getSaveFileName(this, tr("Save RT"), tr("RT.png"), tr("PNG(*.png);;JPG(*.jpg)"));
	std::cout << path.toStdString();
	m_ui.qRayTracingView->grab().scaled(m_width, m_height).save(path);

}
