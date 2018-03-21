#include "../headers/phongglwidget.h"
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QColorDialog>
#include <QPainter>
#include <math.h>

#include <iostream>

PhongGLWidget::PhongGLWidget(QWidget *parent) : QOpenGLWidget(parent)
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

	// Shaders
	m_program = nullptr;

	// FPS
	m_showFps = false;
	


	// TO DO: Initialize the other attributes



}

PhongGLWidget::~PhongGLWidget()
{
    cleanup();
}

QSize PhongGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize PhongGLWidget::sizeHint() const
{
    return QSize(m_width, m_height);
}

void PhongGLWidget::cleanup()
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

void PhongGLWidget::initializeGL()
{
    // In this example the widget's corresponding top-level window can change
    // several times during the widget's lifetime. Whenever this happens, the
    // QOpenGLWidget's associated context is destroyed and a new one is created.
    // Therefore we have to be prepared to clean up the resources on the
    // aboutToBeDestroyed() signal, instead of the destructor. The emission of
    // the signal will be followed by an invocation of initializeGL() where we
    // can recreate all resources.
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &PhongGLWidget::cleanup);
    initializeOpenGLFunctions();
 	loadShaders();
	createBuffersScene();
	computeBBoxScene();
	projectionTransform();
	viewTransform();
}

void PhongGLWidget::paintGL()
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

	// Unbind the vertex array
	glBindVertexArray(0);


	if (m_showFps)
		showFps();
}

void PhongGLWidget::resizeGL(int w, int h)
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

void PhongGLWidget::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_B:
			// Change the background color
			std::cout << "-- AGEn message --: Change background color" << std::endl;
			changeBackgroundColor();
			break;
		case Qt::Key_F:
			// Enable/Disable frames per second
			m_showFps = !m_showFps;
			
			// TO DO: Show or hide the FPS information

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

void PhongGLWidget::mousePressEvent(QMouseEvent *event)
{

	// TO DO: Rotation of the scene and PAN

}

void PhongGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	
	// TO DO: Rotation of the scene and PAN

}

void PhongGLWidget::mouseReleaseEvent(QMouseEvent *event)
{

	// TO DO: Rotation of the scene and PAN

}

void PhongGLWidget::wheelEvent(QWheelEvent* event)
{

	// TO DO: Change the fov of the camera to zoom in and out	

}

void PhongGLWidget::loadShaders()
{
	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./shaders/basicgl.vert");
	fs.compileSourceFile("./shaders/basicgl.frag");

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

void PhongGLWidget::reloadShaders()
{
	if (m_program == nullptr)
		return;
	makeCurrent();
	delete m_program;
	m_program = 0;
	loadShaders();
	update();
}

void PhongGLWidget::projectionTransform()
{
	// Set the camera type
	glm::mat4 proj(1.0f);
	

	// TO DO: Set the camera parameters 
	

	// Send the matrix to the shader
	glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, &proj[0][0]);

}

void PhongGLWidget::resetCamera()
{

	// TO DO: Reset the camera/view parameters

}

void PhongGLWidget::viewTransform()
{
	// Set the camera position
	glm::mat4 view(1.0f);


	// TO DO: Camera placement and PAN
	

	// Send the matrix to the shader
	glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &view[0][0]);
}

void PhongGLWidget::changeBackgroundColor() 
{

	// TO DO: Change the background color

}

void PhongGLWidget::createBuffersScene()
{

	// TO DO: Create the buffers, initialize VAO, VBOs, etc.

}

void PhongGLWidget::computeBBoxScene()
{
	// Right now we have just a quad of 20x20x0
	m_sceneRadius = sqrt(20 * 20 + 20 * 20)/2.0f;
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
}

void PhongGLWidget::sceneTransform()
{
	glm::mat4 geomTransform(1.0f);


	// TO DO: Rotations of the scene


	// Send the matrix to the shader
	glUniformMatrix4fv(m_transLoc, 1, GL_FALSE, &geomTransform[0][0]);
}

void PhongGLWidget::computeFps() 
{

	// TO DO: Compute the FPS

}

void PhongGLWidget::showFps()
{
	
	// TO DO: Show the FPS

}