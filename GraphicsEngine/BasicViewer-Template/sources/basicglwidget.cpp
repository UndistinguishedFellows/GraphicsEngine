#include "..\headers\basicglwidget.h"
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QColorDialog>
#include <QPainter>
#include <math.h>

#include <iostream>

BasicGLWidget::BasicGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
	// To receive key events
	setFocusPolicy(Qt::StrongFocus);

	// Attributes initialization
	// Screen
	m_width = 500;
	m_height = 500;
	
	// Scene
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_sceneRadius = 50.0f;
	m_bkgColor = Qt::black;
	m_backFaceCulling = false;
	m_Interaction = NONE;

	// Camera
	m_cameraType = SIMPLE;

	// Shaders
	m_program = nullptr;

	// FPS
	m_showFps = false;
	m_frameCount = 0;
	m_fps = 0;


	// Initialize the other attributes
	m_ar = m_width / m_height;
	m_fovIni = DEG2RAD(60.0f);
	m_fov = m_fovIni;
	m_radsZoom = 0.0f;
	m_zNear = 0.1f;
	m_zFar = 50.f;

	m_xClick = 0;
	m_yClick = 0;
	m_xPan = 0.f;
	m_yPan = 0.f;
	m_xRot = 0.f;
	m_yRot = 0.f;

}

BasicGLWidget::~BasicGLWidget()
{
    cleanup();
}

QSize BasicGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize BasicGLWidget::sizeHint() const
{
    return QSize(m_width, m_height);
}

void BasicGLWidget::SelectCameraType(CameraType type)
{
	m_cameraType = type;
}

void BasicGLWidget::cleanup()
{
	makeCurrent();
	
	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &m_VBOVerts);
	glDeleteBuffers(1, &m_VBONorms);
	glDeleteBuffers(1, &m_VBOCols);
	glDeleteVertexArrays(1, &m_VAO);
	
	if (m_program == nullptr)
        return;
    
	delete m_program;
    m_program = 0;
    
	doneCurrent();
}

void BasicGLWidget::initializeGL()
{
    // In this example the widget's corresponding top-level window can change
    // several times during the widget's lifetime. Whenever this happens, the
    // QOpenGLWidget's associated context is destroyed and a new one is created.
    // Therefore we have to be prepared to clean up the resources on the
    // aboutToBeDestroyed() signal, instead of the destructor. The emission of
    // the signal will be followed by an invocation of initializeGL() where we
    // can recreate all resources.
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &BasicGLWidget::cleanup);
    initializeOpenGLFunctions();
 	loadShaders();
	createBuffersScene();
	computeBBoxScene();
	projectionTransform();
	viewTransform();
}

void BasicGLWidget::paintGL()
{
	// FPS computation
	computeFps();

	// Paint the scene
	glClearColor(m_bkgColor.red() / 255.0f, m_bkgColor.green() / 255.0f, m_bkgColor.blue() / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	if (m_backFaceCulling)
		glEnable(GL_CULL_FACE);

	// Bind the VAO to draw the scene
	glBindVertexArray(m_VAO);

	// Apply the geometric transforms to the scene (position/orientation)
	sceneTransform();

	// Draw the scene
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, m_EBO, GL_UNSIGNED_INT, NULL);

	// Unbind the vertex array
	glBindVertexArray(0);


	if (m_showFps)
		showFps();
}

void BasicGLWidget::resizeGL(int w, int h)
{
	m_width = w;
	m_height = h;
	
	glViewport(0, 0, m_width, m_height);
	m_ar = (float)m_width / (float)m_height;
	
	// We do this if we want to preserve the initial fov when resizing
	if (m_ar < 1.0f) 
	{
		m_fov = 2.0f*atan(tan(m_fovIni / 2.0f) / m_ar) + m_radsZoom;
	}
	else
	{
		m_fov = m_fovIni + m_radsZoom;
	}

	// After modifying the parameters, we update the camera projection
	projectionTransform();
}

void BasicGLWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) 
	{
		case Qt::Key_B:
			// Change the background color
			std::cout << "-- AGEn message --: Change background color" << std::endl;
			changeBackgroundColor();
			break;
		case Qt::Key_F:
			// Enable/Disable frames per second
			m_showFps = !m_showFps;
			update();

			break;
		case Qt::Key_H:
			// Show the help message
			std::cout << "-- AGEn message --: Help" << std::endl;
			std::cout << std::endl;
			std::cout << "Keys used in the application:" << std::endl;
			std::cout << std::endl;
			std::cout << "-B:  change background color" << std::endl;
			std::cout << "-F:  show frames per second (fps)" << std::endl;
			std::cout << "-H:  show this help" << std::endl;
			std::cout << "-R:  reset the camera parameters" << std::endl;
			std::cout << "-F5: reload shaders" << std::endl;
			std::cout << std::endl;
			std::cout << "IMPORTANT: the focus must be set to the glwidget to work" << std::endl;
			std::cout << std::endl;
			break;
		case Qt::Key_R:
			// Reset the camera and scene parameters
			std::cout << "-- AGEn message --: Reset camera" << std::endl;
			resetCamera();
			break;
		case Qt::Key_F5: 
			// Reload shaders
			std::cout << "-- AGEn message --: Reload shaders" << std::endl;
			reloadShaders();
			break;
		default:
			event->ignore();
			break;
	}
}

void BasicGLWidget::mousePressEvent(QMouseEvent *event)
{
	m_xClick = event->x();
	m_yClick = event->y();

	if (event->buttons() & Qt::LeftButton)
		m_Interaction = ROTATE;
	else if (event->buttons() & Qt::RightButton)
		m_Interaction = PAN;
}

void BasicGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();
	
	if(m_Interaction == ROTATE)
	{
		m_yRot += (event->x() - m_xClick) * PI / 180.f;
		m_xRot += (event->y() - m_yClick) * PI / 180.f;
	}
	else if(m_Interaction == PAN)
	{
		m_xPan += (event->x() - m_xClick) * 0.1f;
		m_yPan += (event->y() - m_yClick) * 0.1f;
		viewTransform();
	}

	m_xClick = event->x();
	m_yClick = event->y();

	update();
}

void BasicGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_Interaction = NONE;
	event->ignore();
}

void BasicGLWidget::wheelEvent(QWheelEvent* event)
{
	int deg = event->delta() / 8;

	float rads = DEG2RAD(deg / 2.0f);

	float maxFov = DEG2RAD(175.f);
	float minFov = DEG2RAD(15.f);

	if(m_fov >= minFov && m_fov <= maxFov)
	{
		makeCurrent();

		float preFov = m_fov;

		float fov = m_fov + rads / 2.f;
		fov = MIN(fov, maxFov);
		m_fov = MAX(fov, minFov);

		m_radsZoom += m_fov - preFov;

		projectionTransform();
		update();
	}

	event->accept();
}

void BasicGLWidget::loadShaders()
{
	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./BasicViewer-Template/shaders/basicgl.vert");
	fs.compileSourceFile("./BasicViewer-Template/shaders/basicgl.frag");

	// Create the program
	m_program = new QOpenGLShaderProgram;

	// Add the shaders
	m_program->addShader(&fs);
	m_program->addShader(&vs);

	// Link the program
	m_program->link();

	// Bind the program (we are gonna use this program)
	m_program->bind();

	// Get the attribs locations of the vertex shader
	m_vertexLoc = glGetAttribLocation(m_program->programId(), "vertex");
	m_normalLoc = glGetAttribLocation(m_program->programId(), "normal");
	m_colorLoc = glGetAttribLocation(m_program->programId(), "color");

	// Get the uniforms locations of the vertex shader
	m_transLoc = glGetUniformLocation(m_program->programId(), "sceneTransform");
	m_projLoc = glGetUniformLocation(m_program->programId(), "projTransform");
	m_viewLoc = glGetUniformLocation(m_program->programId(), "viewTransform");
}

void BasicGLWidget::reloadShaders()
{
	if (m_program == nullptr)
		return;
	makeCurrent();
	delete m_program;
	m_program = 0;
	loadShaders();
	update();
}

void BasicGLWidget::projectionTransform()
{
	// Set the camera type
	glm::mat4 proj(1.0f);
	
	m_zNear = m_sceneRadius;
	m_zFar = 3.0f * m_sceneRadius;
	
	proj = glm::perspective(m_fov, m_ar, m_zNear, m_zFar);

	// Send the matrix to the shader
	glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, &proj[0][0]);
}

void BasicGLWidget::resetCamera()
{
	makeCurrent();

	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_ar = m_width / m_height;
	m_fov = m_fovIni;
	m_radsZoom = 0.0f;
	m_zNear = 0.1f;
	m_zFar = 50.f;

	m_xClick = 0;
	m_yClick = 0;
	m_xPan = 0.f;
	m_yPan = 0.f;
	m_xRot = 0.f;
	m_yRot = 0.f;

	viewTransform();
	projectionTransform();
	repaint();
}

void BasicGLWidget::viewTransform()
{
	makeCurrent();
	// Set the camera position
	glm::mat4 view(1.0f);

	view = glm::translate(view, m_sceneCenter + glm::vec3(0.f, 0.f, -2.f * m_sceneRadius));
	view = glm::translate(view, glm::vec3(m_xPan, -m_yPan, 0.f));
	view = glm::translate(view, -m_sceneCenter);

	// Send the matrix to the shader
	glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &view[0][0]);
}

void BasicGLWidget::changeBackgroundColor() 
{
	m_bkgColor = QColorDialog::getColor();
	repaint();
}

void BasicGLWidget::createBuffersScene()
{
	// VBO vertices positions
	glm::vec3 verts[6] = {
		glm::vec3(-10.f, -10.f, 0.f),
		glm::vec3(-10.f, 10.f, 0.f),
		glm::vec3(10.f, -10.f, 0.f),
		glm::vec3(10.f, -10.f, 0.f),
		glm::vec3(-10.f, 10.f, 0.f),
		glm::vec3(10.f, 10.f, 0.f)
	};

	// VBO normals
	glm::vec3 normVerts[6] = {
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f)
	};

	// VBO colors
	glm::vec4 colorsVerts[6] = {
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)
	};

	// VAO creation
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	// VBO vertices
	glGenBuffers(1, &m_VBOVerts);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOVerts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Enable attribute m_vertexLoc
	glVertexAttribPointer(m_vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_vertexLoc);

	// VBO normals
	glGenBuffers(1, &m_VBONorms);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBONorms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normVerts), normVerts, GL_STATIC_DRAW);

	// Enable the attribute m_normalLoc
	glVertexAttribPointer(m_normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_normalLoc);

	// VBO Colors
	glGenBuffers(1, &m_VBOCols);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOCols);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colorsVerts), colorsVerts, GL_STATIC_DRAW);

	// Enable the attribute m_colorLoc
	glVertexAttribPointer(m_colorLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_colorLoc);

	glBindVertexArray(0);
}

void BasicGLWidget::computeBBoxScene()
{
	// Right now we have just a quad of 20x20x0
	m_sceneRadius = sqrt(20 * 20 + 20 * 20)/2.0f;
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
}

void BasicGLWidget::sceneTransform()
{
	glm::mat4 geomTransform(1.0f);

	geomTransform = glm::translate(geomTransform, m_sceneCenter);
	geomTransform = glm::rotate(geomTransform, m_xRot, glm::vec3(-1.f, 0.f, 0.f));
	geomTransform = glm::rotate(geomTransform, m_yRot, glm::vec3(0.f, 1.f, 0.f));
	geomTransform = glm::translate(geomTransform, -m_sceneCenter);

	// Send the matrix to the shader
	glUniformMatrix4fv(m_transLoc, 1, GL_FALSE, &geomTransform[0][0]);
}

void BasicGLWidget::computeFps() 
{
	if (m_frameCount == 0)
		m_timer.start();

	if (m_timer.elapsed() / 1000.f >= 1.f)
	{
		m_fps = m_frameCount;
		m_frameCount = 0;
		m_timer.restart();
	}

	++m_frameCount;
}

void BasicGLWidget::showFps()
{
	makeCurrent();

	if (m_backFaceCulling)
		glDisable(GL_CULL_FACE);

	m_program->release();

	QPainter p;
	p.begin(this);

	p.setPen(QColor(255, 255, 255));

	QString text(tr(std::to_string(m_fps).c_str()));

	p.fillRect(0, 0, 50, 40, QColor(0, 0, 0, 255));
	p.drawText(10, 10, 40, 30, Qt::AlignCenter, text);

	p.end();

	if (m_backFaceCulling)
		glEnable(GL_CULL_FACE);

	m_program->bind();

	update();
}