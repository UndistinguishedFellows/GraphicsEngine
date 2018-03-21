#include "../headers/basicglwidget.h"
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
	
	// Camera
	m_ar = 1.0f;
	m_fov = PI / 3.0f;
	m_fovIni = m_fov;
	m_zNear = 0.1f;
	m_zFar = 100.0f;
	m_radsZoom = 0.0f;
	m_xPan = 0.0f;
	m_yPan = 0.0f;

	// Scene
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_sceneRadius = 50.0f;
	m_bkgColor = Qt::black;
	m_backFaceCulling = false;

	// Mouse
	m_xRot = 0.0f;
	m_yRot = 0.0f;
	m_xClick = 0;
	m_yClick = 0;
	m_doingInteractive = NONE;
	
	// Shaders
	m_program = nullptr;
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

	// Unbind the vertex array
	glBindVertexArray(0);
}

void BasicGLWidget::resizeGL(int w, int h)
{
	m_width = w;
	m_height = h;
	
	glViewport(0, 0, m_width, m_height);
	m_ar = (float)m_width / (float)m_height;
	
	// We do this if we want to preserve the initial fov when resizing
	if (m_ar < 1.0f) {
		m_fov = 2.0f*atan(tan(m_fovIni / 2.0f) / m_ar) + m_radsZoom;
	}
	else {
		m_fov = m_fovIni + m_radsZoom;
	}

	// After modifying the parameters, we update the camera projection
	projectionTransform();
}

void BasicGLWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_B:
			// Change the background color
			std::cout << "-- AGEn message --: Change background color" << std::endl;
			changeBackgroundColor();
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
			std::cout << std::endl;
			std::cout << "IMPORTANT: the focus must be set to the glwidget to work" << std::endl;
			std::cout << std::endl;
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

	if (event->buttons() & Qt::LeftButton) {
		m_doingInteractive = ROTATE;
	}
	else if (event->buttons() & Qt::RightButton){
		m_doingInteractive = PAN;
	}
}

void BasicGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();

	if (m_doingInteractive == ROTATE)
	{
		m_yRot += (event->x() - m_xClick) * PI / 180.0f;
		m_xRot += (event->y() - m_yClick) * PI / 180.0f;
	}
	else if (m_doingInteractive == PAN) {
		m_xPan += (event->x() - m_xClick)*0.1f;
		m_yPan += (event->y() - m_yClick)*0.1f;
		viewTransform();
	}

	m_xClick = event->x();
	m_yClick = event->y();

	update();
}

void BasicGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_doingInteractive = NONE;
	event->ignore();
}

void BasicGLWidget::wheelEvent(QWheelEvent* event)
{
	// In many mouses, each step of the wheel is considered 15 degs.
	int numDegrees = event->delta() / 8;
	
	// Number of steps of the wheel
	//int numSteps = numDegrees / 15;

	// For each step, we zoom in or out 7.5 degrees
	float numRads = DEG2RAD(numDegrees/2.0f);

	float maxFov = DEG2RAD(175.0f);
	float minFov = DEG2RAD(15.0f);

	if (m_fov >= minFov && m_fov <= maxFov) {
		
		makeCurrent();
		
		float fovBeforeZoom = m_fov;
		
		float newFov = m_fov + numRads/2.0f;
		newFov = MIN(newFov, maxFov);
		m_fov = MAX(newFov, minFov);

		// Total change of the fov to resize well the screen
		m_radsZoom += m_fov - fovBeforeZoom;

		// After modifying the parameters, we update the camera projection
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
	vs.compileSourceFile("./shaders/geomshaderdemo.vert");
	fs.compileSourceFile("./shaders/geomshaderdemo.frag");

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

void BasicGLWidget::viewTransform()
{
	// Set the camera position
	glm::mat4 view(1.0f);

	view = glm::translate(view, m_sceneCenter + glm::vec3(0.0f, 0.0f, -2.0f * m_sceneRadius));
	view = glm::translate(view, glm::vec3(m_xPan, -m_yPan, 0.0f));
	view = glm::translate(view, -m_sceneCenter);
	
	// Send the matrix to the shader
	glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &view[0][0]);
}

void BasicGLWidget::changeBackgroundColor() {
	
	m_bkgColor = QColorDialog::getColor();
	repaint();
}

void BasicGLWidget::createBuffersScene()
{
	// Simple geometric scene
	// VBO vertices positions
	glm::vec3 verts[6] = {
		glm::vec3(-10.0f, -10.0f, 0.0f),
		glm::vec3(-10.0f, 10.0f, 0.0f),
		glm::vec3(10.0f, -10.0f, 0.0f),
		glm::vec3(10.0f, -10.0f, 0.0f),
		glm::vec3(-10.0f, 10.0f, 0.0f),
		glm::vec3(10.0f, 10.0f, 0.0f)
	};

	// VBO normals
	glm::vec3 normVerts[6] = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
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

	// VBO Vertices
	glGenBuffers(1, &m_VBOVerts);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOVerts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Enable the attribute m_vertexLoc
	glVertexAttribPointer(m_vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_vertexLoc);

	// VBO Normals
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
	geomTransform = glm::rotate(geomTransform, m_xRot, glm::vec3(-1.0f, 0.0f, 0.0f));
	geomTransform = glm::rotate(geomTransform, m_yRot, glm::vec3(0.0f, 1.0f, 0.0f));
	geomTransform = glm::translate(geomTransform, -m_sceneCenter);

	// Send the matrix to the shader
	glUniformMatrix4fv(m_transLoc, 1, GL_FALSE, &geomTransform[0][0]);
}
