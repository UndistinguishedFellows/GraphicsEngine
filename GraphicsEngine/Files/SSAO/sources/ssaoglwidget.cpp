#include "../headers/SSAOGLWidget.h"
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QColorDialog>
#include <QMessageBox>
#include <QPainter>
#include <math.h>
#include <QOpenGLFramebufferObjectFormat>

#include <iostream>
#include <random>

SSAOGLWidget::SSAOGLWidget(QString modelFilename, bool showFps, QWidget *parent) : QOpenGLWidget(parent)
{
	// To receive key events
	setFocusPolicy(Qt::StrongFocus);

	this->installEventFilter(this);

	// Attributes initialization
	// Screen
	m_width = 800;
	m_height = 600;
	// Mouse
	m_xRot = 0.0f;
	m_yRot = 0.0f;
	m_xClick = 0;
	m_yClick = 0;
	m_doingInteractive = NONE;
	// Scene
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_sceneRadius = 50.0f;
	m_bkgColor = Qt::black;
	m_backFaceCulling = true;
	// Camera
	m_ar = 1.0f;
	m_fov = PI / 3.0f;
	m_fovIni = m_fov;
	m_zNear = 0.1f;
	m_zFar = 100.0f;
	m_radsZoom = 0.0f;
	m_xPan = 0.0f;
	m_yPan = 0.0f;

	m_moveSpeed = 5.0f;
	m_mouseSensitivity = 0.25f;
	m_camPos = glm::vec3(0.f, 0.f, 1.f);
	m_camFront = glm::vec3(0.f, 0.f, -1.f);
	m_camUp = glm::vec3(0.f, 1.f, 0.f);
	m_Pitch = 0.0f;
	m_Yaw = 0.0f;
	m_constrainPitch = false;
	updateCameraVectors();

	// Scene
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_sceneRadius = 50.0f;
	m_bkgColor = Qt::black;
	m_backFaceCulling = true;

	// Model
	m_modelLoaded = false;
	m_modelCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	m_modelRadius = 0.0f;
	m_modelFilename = modelFilename;

	// FPS
	m_frameCount = 0;
	m_fps = 0;
	m_showFps = showFps;

	// Shaders
	m_GProgram.m_program = nullptr;
	m_SSAOProgram.m_program = nullptr;
	m_SSAOBlurProgram.m_program = nullptr;
	m_basciProgram.m_program = nullptr;
}

SSAOGLWidget::~SSAOGLWidget()
{
	cleanup();
}

QSize SSAOGLWidget::minimumSizeHint() const
{
	return QSize(50, 50);
}

QSize SSAOGLWidget::sizeHint() const
{
	return QSize(m_width, m_height);
}

void SSAOGLWidget::cleanup()
{
	if (m_modelLoaded)
		cleanBuffersModel();

	if (m_GProgram.m_program == nullptr || m_basciProgram.m_program == nullptr || m_SSAOProgram.m_program == nullptr || m_SSAOBlurProgram.m_program == nullptr)
		return;

	makeCurrent();

	delete m_GProgram.m_program;
	m_GProgram.m_program = 0;

	delete m_basciProgram.m_program;
	m_basciProgram.m_program = 0;

	delete m_SSAOProgram.m_program;
	m_SSAOProgram.m_program = 0;

	delete m_SSAOBlurProgram.m_program;
	m_SSAOBlurProgram.m_program = 0;

	doneCurrent();
}

void SSAOGLWidget::initializeGL()
{
	// In this example the widget's corresponding top-level window can change
	// several times during the widget's lifetime. Whenever this happens, the
	// QOpenGLWidget's associated context is destroyed and a new one is created.
	// Therefore we have to be prepared to clean up the resources on the
	// aboutToBeDestroyed() signal, instead of the destructor. The emission of
	// the signal will be followed by an invocation of initializeGL() where we
	// can recreate all resources.
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &SSAOGLWidget::cleanup);
	initializeOpenGLFunctions();
	loadShaders();
	createQuad();
	loadModel();
	createFramebuffers();
	computeBBoxModel();
	computeCenterRadiusScene();
	createSampleKernel();
	createNoiseTexture();
	initCameraParams();
	projectionTransform();
	viewTransform();
	setLighting();

	printHelp();
}

void SSAOGLWidget::resizeGL(int w, int h)
{
	m_width = w;
	m_height = h;

	m_SSAOProgram.m_program->bind();
	glUniform1f(m_SSAOBlurProgram.m_wScreenLoc, m_width);
	glUniform1f(m_SSAOBlurProgram.m_hScreenLoc, m_height);

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

void SSAOGLWidget::keyPressEvent(QKeyEvent *event)
{
	float speed = m_moveSpeed;

	switch (event->key())
	{
	case Qt::Key_W:
		m_camPos += m_camFront * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_S:
		m_camPos -= m_camFront * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_A:
		m_camPos -= m_camRight * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_D:
		m_camPos += m_camRight * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_Q:
		m_camPos -= m_camUp * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_E:
		m_camPos += m_camUp * speed;
		viewTransform();
		update();
		break;
	case Qt::Key_B:
		// Change the background color
		std::cout << "-- AGEn message --: Change background color" << std::endl;
		changeBackgroundColor();
		break;
	case Qt::Key_C:
		// Set the camera at the center of the scene
		std::cout << "-- AGEn message --: Centering camera" << std::endl;
		resetCamera(true);
		// TO DO: When pressing the C key, the camera must be placed at the center of the scene automatically
		break;
	case Qt::Key_F:
		// Enable/Disable frames per second
		m_showFps = !m_showFps;
		repaint();
		break;
	case Qt::Key_H:
		// Show the help message
		printHelp();
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

void SSAOGLWidget::mousePressEvent(QMouseEvent *event)
{
	m_xClick = event->x();
	m_yClick = event->y();

	if (event->buttons() & Qt::LeftButton)
		m_doingInteractive = ROTATE;
	else if (event->buttons() & Qt::RightButton)
		m_doingInteractive = PAN;
}

void SSAOGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	makeCurrent();

	if (m_doingInteractive == ROTATE)
	{
		m_yRot += (event->x() - m_xClick) * PI / 180.f;
		m_xRot += (event->y() - m_yClick) * PI / 180.f;

		if (m_cameraType == FPS)
		{
			float xoffset = (event->x() - m_xClick) * m_mouseSensitivity;
			float yoffset = -(event->y() - m_yClick) * m_mouseSensitivity;

			m_Yaw += xoffset;
			m_Pitch += yoffset;

			if (m_constrainPitch)
			{
				if (m_Pitch > 89.0f) m_Pitch = 89.0f;
				if (m_Pitch < -89.0f) m_Pitch = -89.0f;
			}

			updateCameraVectors();
			viewTransform();
		}
	}
	else if (m_doingInteractive == PAN)
	{
		m_xPan += (event->x() - m_xClick) * 0.1f;
		m_yPan += (event->y() - m_yClick) * 0.1f;
		viewTransform();
	}

	m_xClick = event->x();
	m_yClick = event->y();

	update();
}

void SSAOGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_doingInteractive = NONE;
	event->ignore();
}

void SSAOGLWidget::wheelEvent(QWheelEvent* event)
{
	int deg = event->delta() / 8;

	float rads = DEG2RAD(deg / 2.0f);

	float maxFov = DEG2RAD(175.f);
	float minFov = DEG2RAD(15.f);

	if (m_fov >= minFov && m_fov <= maxFov)
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

void SSAOGLWidget::printHelp()
{
	std::cout << "-- AGEn message --: Help" << std::endl;
	std::cout << std::endl;
	std::cout << "Keys used in the application:" << std::endl;
	std::cout << std::endl;
	std::cout << "-B:  change background color" << std::endl;
	std::cout << "-C:  set the camera at the center of the scene" << std::endl;
	std::cout << "-F:  show frames per second (fps)" << std::endl;
	std::cout << "-H:  show this help" << std::endl;
	std::cout << "-R:  reset the camera parameters" << std::endl;
	std::cout << "-F5: reload shaders" << std::endl;
	std::cout << std::endl;
	std::cout << "IMPORTANT: the focus must be set to the glwidget to work" << std::endl;
	std::cout << std::endl;
}

void SSAOGLWidget::loadShaders()
{
	std::cout << "===Loading shaders..." << std::endl;

	loadGShader();
	loadSSAOShader();
	loadSSAOBlurShader();
	loadBasicShader();

	std::cout << "=== Shaders loaded." << std::endl;
}

void SSAOGLWidget::loadGShader()
{
	std::cout << "\tLoading gprogram shader" << std::endl;

	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./Files/SSAO/shaders/ssao_geometry.vert");
	fs.compileSourceFile("./Files/SSAO/shaders/ssao_geometry.frag");

	// Create the program
	m_GProgram.m_program = new QOpenGLShaderProgram;

	// Add the shaders
	m_GProgram.m_program->addShader(&vs);
	m_GProgram.m_program->addShader(&fs);

	// Link the program
	m_GProgram.m_program->link();

	// Bind the program (we are gonna use this program)
	m_GProgram.m_program->bind();

	// Get the attribs locations of the vertex shader
	m_GProgram.m_VertexLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "vertex");
	m_GProgram.m_NormalLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "normal");
	m_GProgram.m_matAmbLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "matamb");
	m_GProgram.m_matDiffLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "matdiff");
	m_GProgram.m_matSpecLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "matspec");
	m_GProgram.m_matShinLoc = glGetAttribLocation(m_GProgram.m_program->programId(), "matshin");

	// Get the uniforms locations of the vertex shader
	m_GProgram.m_transLoc = glGetUniformLocation(m_GProgram.m_program->programId(), "sceneTransform");
	m_GProgram.m_projLoc = glGetUniformLocation(m_GProgram.m_program->programId(), "projTransform");
	m_GProgram.m_viewLoc = glGetUniformLocation(m_GProgram.m_program->programId(), "viewTransform");
	m_GProgram.m_lightPosLoc = glGetUniformLocation(m_GProgram.m_program->programId(), "lightPos");
	m_GProgram.m_lightColLoc = glGetUniformLocation(m_GProgram.m_program->programId(), "lightCol");
}

void SSAOGLWidget::loadSSAOShader()
{
	std::cout << "\tLoading ssao shader" << std::endl;

	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./Files/SSAO/shaders/ssao.vert");
	fs.compileSourceFile("./Files/SSAO/shaders/ssao.frag");

	// Create the program
	m_SSAOProgram.m_program = new QOpenGLShaderProgram;

	// Add the shaders
	m_SSAOProgram.m_program->addShader(&vs);
	m_SSAOProgram.m_program->addShader(&fs);

	// Link the program
	m_SSAOProgram.m_program->link();

	// Bind the program (we are gonna use this program)
	m_SSAOProgram.m_program->bind();

	// Get the attribs locations of the vertex shader
	m_SSAOBlurProgram.m_wScreenLoc = glGetUniformLocation(m_SSAOProgram.m_program->programId(), "screenWidth");
	m_SSAOBlurProgram.m_hScreenLoc = glGetUniformLocation(m_SSAOProgram.m_program->programId(), "screenHeight");

	// Get the uniforms locations of the vertex shader
	m_SSAOProgram.m_ssaoPosTexLoc = glGetUniformLocation(m_SSAOProgram.m_program->programId(), "gPosition");
	m_SSAOProgram.m_ssaoNTexLoc = glGetUniformLocation(m_SSAOProgram.m_program->programId(), "gNormal");
	m_SSAOProgram.m_ssaoBlurLoc = glGetUniformLocation(m_SSAOProgram.m_program->programId(), "texNoise");

	for (unsigned int i = 0; i < 64; ++i)
	{
		std::string name = "samples[" + std::to_string(i) + "]";
		m_SSAOProgram.m_ssaoSampleKernelLoc[i] = glGetUniformLocation(m_SSAOProgram.m_program->programId(), name.c_str());
	}
}

void SSAOGLWidget::loadSSAOBlurShader()
{
	std::cout << "\tLoading ssao blur shader" << std::endl;

	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./Files/SSAO/shaders/ssao.vert");
	fs.compileSourceFile("./Files/SSAO/shaders/ssao_blur.frag");

	// Create the program
	m_SSAOBlurProgram.m_program = new QOpenGLShaderProgram;

	// Add the shaders
	m_SSAOBlurProgram.m_program->addShader(&vs);
	m_SSAOBlurProgram.m_program->addShader(&fs);

	// Link the program
	m_SSAOBlurProgram.m_program->link();

	// Bind the program (we are gonna use this program)
	m_SSAOBlurProgram.m_program->bind();

	// Get the uniforms locations of the vertex shader
	m_SSAOBlurProgram.m_ssaoTextureLoc = glGetUniformLocation(m_SSAOBlurProgram.m_program->programId(), "ssaoInput");
}

void SSAOGLWidget::loadBasicShader()
{
	std::cout << "\tLoading basic shader" << std::endl;

	// Declaration of the shaders
	QOpenGLShader vs(QOpenGLShader::Vertex, this);
	QOpenGLShader fs(QOpenGLShader::Fragment, this);

	// Load and compile the shaders
	vs.compileSourceFile("./Files/SSAO/shaders/ssao.vert");
	fs.compileSourceFile("./Files/SSAO/shaders/ssao_lighting.frag");

	// Create the program
	m_basciProgram.m_program = new QOpenGLShaderProgram;

	// Add the shaders
	m_basciProgram.m_program->addShader(&vs);
	m_basciProgram.m_program->addShader(&fs);

	// Link the program
	m_basciProgram.m_program->link();

	// Bind the program (we are gonna use this program)
	m_basciProgram.m_program->bind();

	// Get the uniforms locations of the vertex shader
	m_basciProgram.m_basicLoc = glGetUniformLocation(m_basciProgram.m_program->programId(), "renderType");
	m_basciProgram.m_phongTexLoc = glGetUniformLocation(m_basciProgram.m_program->programId(), "gPhong");
	m_basciProgram.m_ssaoTexLoc = glGetUniformLocation(m_basciProgram.m_program->programId(), "ssao");
	m_basciProgram.m_ssaoNormalLoc = glGetUniformLocation(m_basciProgram.m_program->programId(), "normal");
	m_basciProgram.m_ssaogPositionLoc = glGetUniformLocation(m_basciProgram.m_program->programId(), "gPosition");
}

void SSAOGLWidget::reloadShaders()
{
	if (m_GProgram.m_program == nullptr || m_basciProgram.m_program == nullptr || m_SSAOProgram.m_program == nullptr || m_SSAOBlurProgram.m_program == nullptr)
		return;

	makeCurrent();

	delete m_GProgram.m_program;
	m_GProgram.m_program = 0;

	delete m_basciProgram.m_program;
	m_basciProgram.m_program = 0;

	delete m_SSAOProgram.m_program;
	m_SSAOProgram.m_program = 0;

	delete m_SSAOBlurProgram.m_program;
	m_SSAOBlurProgram.m_program = 0;

	loadShaders();

	doneCurrent();
}

void SSAOGLWidget::initCameraParams()
{
	m_cameraType = FPS;
	resetCamera(true);
}

void SSAOGLWidget::projectionTransform()
{
	// Set the camera type
	glm::mat4 proj(1.0f);

	proj = glm::perspective(m_fov, m_ar, m_zNear, m_zFar);

	// Send the matrix to the shader
	m_SSAOProgram.m_program->bind();
	glUniformMatrix4fv(glGetUniformLocation(m_SSAOProgram.m_program->programId(), "projTransform"), 1, GL_FALSE, &proj[0][0]);
	m_GProgram.m_program->bind();
	glUniformMatrix4fv(glGetUniformLocation(m_GProgram.m_program->programId(), "projTransform"), 1, GL_FALSE, &proj[0][0]);
}

void SSAOGLWidget::resetCamera(bool onSceneCenter)
{
	makeCurrent();
	m_radsZoom = 0.0f;
	m_xPan = 0.0f;
	m_yPan = 0.0f;
	m_camPos = onSceneCenter ? glm::vec3(0.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, -2.0f * m_sceneRadius);
	m_camFront = glm::vec3(0.f, 0.f, -1.f);
	m_camUp = glm::vec3(0.f, 1.f, 0.f);
	m_Pitch = 0.0f;
	m_Yaw = 0.0f;
	m_constrainPitch = false;
	updateCameraVectors();

	m_zNear = 0.5f;
	m_zFar = 150.0f;

	if (m_ar < 1.0f) 
	{
		m_fov = 2.0f*atan(tan(m_fovIni / 2.0f) / m_ar) + m_radsZoom;
	}
	else 
	{
		m_fov = m_fovIni + m_radsZoom;
	}

	m_xRot = 0.0f;
	m_yRot = 0.0f;

	projectionTransform();
	viewTransform();
	update();
}

void SSAOGLWidget::viewTransform()
{
	glm::mat4 view(1.0f);

	if (m_cameraType == STATIC)
	{
		view = glm::translate(view, m_sceneCenter + m_camPos);
		view = glm::translate(view, glm::vec3(m_xPan, -m_yPan, 0.0f));
		view = glm::translate(view, -m_sceneCenter);
	}
	else if(m_cameraType == FPS)
	{
		view = glm::lookAt(m_camPos, m_camPos + m_camFront, glm::vec3(0.f, 1.f, 0.f));
	}

	// Send the matrix to the shader
	m_GProgram.m_program->bind();
	glUniformMatrix4fv(m_GProgram.m_viewLoc, 1, GL_FALSE, &view[0][0]);
}

void SSAOGLWidget::changeBackgroundColor() {

	m_bkgColor = QColorDialog::getColor();
	repaint();
}

void SSAOGLWidget::computeCenterRadiusScene()
{
	m_sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);

	// In this case, we just load one model
	m_sceneRadius = m_modelRadius;
}

void SSAOGLWidget::createQuad()
{
	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	glGenVertexArrays(1, &m_quadVAO);
	glBindVertexArray(m_quadVAO);

	// setup plane VAO
	glGenBuffers(1, &m_quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void SSAOGLWidget::updateCameraVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	front.y = sin(glm::radians(m_Pitch));
	front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

	m_camFront = glm::normalize(front);
	m_camRight = glm::normalize(glm::cross(m_camFront, glm::vec3(0.f, 1.f, 0.f)));
	m_camUp = glm::normalize(glm::cross(m_camRight, m_camFront));
}

void SSAOGLWidget::loadModel()
{
	m_GProgram.m_program->bind();

	std::cout << "--- Loading model: " << m_modelFilename.toStdString() << std::endl;

	// Load the OBJ model - BEFORE creating the buffers!
	m_model.load(m_modelFilename.toStdString());

	// VAO creation
	glGenVertexArrays(1, &m_VAOModel);
	glBindVertexArray(m_VAOModel);

	// VBO Vertices
	glGenBuffers(1, &m_VBOModelVerts);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelVerts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3 * 3, m_model.VBO_vertices(), GL_STATIC_DRAW);

	// Enable the attribute m_vertexLoc
	glVertexAttribPointer(m_GProgram.m_VertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_VertexLoc);

	// VBO Normals
	glGenBuffers(1, &m_VBOModelNorms);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelNorms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3 * 3, m_model.VBO_normals(), GL_STATIC_DRAW);

	// Enable the attribute m_normalLoc
	glVertexAttribPointer(m_GProgram.m_NormalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_NormalLoc);

	//// Instead of colors, we pass the materials 
	// VBO Ambient component
	glGenBuffers(1, &m_VBOModelMatAmb);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelMatAmb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3 * 3, m_model.VBO_matamb(), GL_STATIC_DRAW);

	// Enable the attribute m_matAmbLoc
	glVertexAttribPointer(m_GProgram.m_matAmbLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_matAmbLoc);

	// VBO Diffuse component
	glGenBuffers(1, &m_VBOModelMatDiff);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelMatDiff);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3 * 3, m_model.VBO_matdiff(), GL_STATIC_DRAW);

	// Enable the attribute m_matDiffLoc
	glVertexAttribPointer(m_GProgram.m_matDiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_matDiffLoc);

	// VBO Specular component
	glGenBuffers(1, &m_VBOModelMatSpec);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelMatSpec);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3 * 3, m_model.VBO_matspec(), GL_STATIC_DRAW);

	// Enable the attribute m_matSpecLoc
	glVertexAttribPointer(m_GProgram.m_matSpecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_matSpecLoc);

	// VBO Shininess component
	glGenBuffers(1, &m_VBOModelMatShin);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOModelMatShin);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*m_model.faces().size() * 3, m_model.VBO_matshin(), GL_STATIC_DRAW);

	// Enable the attribute m_matShinLoc
	glVertexAttribPointer(m_GProgram.m_matShinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(m_GProgram.m_matShinLoc);

	glBindVertexArray(0);

	// The model has been loaded
	m_modelLoaded = true;

	std::cout << "---Model loaded" << std::endl;
}

void SSAOGLWidget::cleanBuffersModel()
{
	makeCurrent();

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &m_VBOModelVerts);
	glDeleteBuffers(1, &m_VBOModelNorms);
	glDeleteBuffers(1, &m_VBOModelMatAmb);
	glDeleteBuffers(1, &m_VBOModelMatDiff);
	glDeleteBuffers(1, &m_VBOModelMatSpec);
	glDeleteBuffers(1, &m_VBOModelMatShin);
	glDeleteVertexArrays(1, &m_VAOModel);
	glDeleteBuffers(1, &m_quadVBO);
	glDeleteVertexArrays(1, &m_quadVAO);

	m_modelLoaded = false;

	doneCurrent();
}

void SSAOGLWidget::computeBBoxModel()
{
	float minX, minY, minZ;
	float maxX, maxY, maxZ;

	minX = maxX = m_model.vertices()[0];
	minY = maxY = m_model.vertices()[1];
	minZ = maxZ = m_model.vertices()[2];

	for (size_t i = 3; i < m_model.vertices().size(); i += 3)
	{
		if (m_model.vertices()[i + 0] < minX)
			minX = m_model.vertices()[i + 0];
		if (m_model.vertices()[i + 0] > maxX)
			maxX = m_model.vertices()[i + 0];
		if (m_model.vertices()[i + 1] < minY)
			minY = m_model.vertices()[i + 1];
		if (m_model.vertices()[i + 1] > maxY)
			maxY = m_model.vertices()[i + 1];
		if (m_model.vertices()[i + 2] < minZ)
			minZ = m_model.vertices()[i + 2];
		if (m_model.vertices()[i + 2] > maxZ)
			maxZ = m_model.vertices()[i + 2];
	}

	m_modelCenter = glm::vec3((maxX + minX) / 2.0f, (maxY + minY) / 2.0f, (maxZ + minZ) / 2.0f);
	glm::vec3 radiusModel(maxX - m_modelCenter.x, maxY - m_modelCenter.y, maxZ - m_modelCenter.z);
	m_modelRadius = sqrt(radiusModel.x*radiusModel.x + radiusModel.y*radiusModel.y + radiusModel.z*radiusModel.z);
}

void SSAOGLWidget::modelTransform()
{
	glm::mat4 geomTransform(1.0f);

	if (m_cameraType == STATIC)
	{
		geomTransform = glm::translate(geomTransform, m_sceneCenter);
		geomTransform = glm::rotate(geomTransform, m_xRot, glm::vec3(1.0f, 0.0f, 0.0f));
		geomTransform = glm::rotate(geomTransform, m_yRot, glm::vec3(0.0f, 1.0f, 0.0f));
		geomTransform = glm::translate(geomTransform, -m_modelCenter);
	}
	geomTransform = glm::scale(geomTransform, glm::vec3(0.05f, 0.05f, 0.05f));

	// Send the matrix to the shader
	glUniformMatrix4fv(m_GProgram.m_transLoc, 1, GL_FALSE, &geomTransform[0][0]);
}

void SSAOGLWidget::computeFps()
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

void SSAOGLWidget::showFps()
{
	makeCurrent();

	if (m_backFaceCulling)
		glDisable(GL_CULL_FACE);

	//m_program->release();

	QPainter p;
	p.begin(this);

	p.setPen(QColor(255, 255, 255));

	QString text(tr(std::to_string(m_fps).c_str()));

	p.fillRect(0, 0, 50, 40, QColor(0, 0, 0, 255));
	p.drawText(10, 10, 40, 30, Qt::AlignCenter, text);

	p.end();

	if (m_backFaceCulling)
		glEnable(GL_CULL_FACE);

	//m_program->bind();

	update();
}

void SSAOGLWidget::createSampleKernel()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;

	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0;

		scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}
}

void SSAOGLWidget::createNoiseTexture()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;

	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}

	//m_SSAOProgram.m_program->bind();
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void SSAOGLWidget::createFramebuffers()
{
	QOpenGLFramebufferObjectFormat formatgBuffer;
	formatgBuffer.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

	// gBuffer
	m_gBuffer = new QOpenGLFramebufferObject(m_width, m_height, formatgBuffer);
	m_gBuffer->addColorAttachment(m_width, m_height);
	m_gBuffer->addColorAttachment(m_width, m_height);
	m_gBuffer->addColorAttachment(m_width, m_height);
	m_gBuffer->addColorAttachment(m_width, m_height);
}

void SSAOGLWidget::paintGL()
{
	// FPS computation
	computeFps();

	renderGBuffer();
	renderSSAO();
	renderSSAOBlur();
	renderLight();

	// Show FPS if they are enabled 
	if (m_showFps)
		showFps();
}

void SSAOGLWidget::renderGBuffer()
{
	makeCurrent();

	// Bind GBuffer to save textures (positions, normals and albedo)
	m_gBuffer->bind();
	GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, bufs);

	// Paint the scene
	glClearColor(m_bkgColor.red() / 255.0f, m_bkgColor.green() / 255.0f, m_bkgColor.blue() / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (m_backFaceCulling)
		glEnable(GL_CULL_FACE);

	m_GProgram.m_program->bind();

	// Bind the VAO to draw the model
	glBindVertexArray(m_VAOModel);

	// Camera
	if (m_cameraType == FPS)
	{
		viewTransform();
		update();
	}

	// Apply the geometric transforms to the model (position/orientation)
	modelTransform();

	// Draw the model
	glDrawArrays(GL_TRIANGLES, 0, m_model.faces().size() * 3);

	// Unbind the vertex array	
	glBindVertexArray(0);

	m_gBuffer->release();
}

void SSAOGLWidget::renderSSAO()
{
	makeCurrent();

	m_gBuffer->bind();
	GLenum bufs[] = { GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(1, bufs);

	glClearColor(m_bkgColor.red() / 255.0f, m_bkgColor.green() / 255.0f, m_bkgColor.blue() / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_SSAOProgram.m_program->bind();
	glBindVertexArray(m_quadVAO);
	QVector<uint> texIDs = m_gBuffer->textures();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texIDs[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(m_SSAOProgram.m_ssaoPosTexLoc, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texIDs[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glUniform1i(m_SSAOProgram.m_ssaoNTexLoc, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glUniform1i(m_SSAOProgram.m_ssaoBlurLoc, 3);

	for (unsigned int i = 0; i < 64; ++i)
		glUniform3fv(m_SSAOProgram.m_ssaoSampleKernelLoc[i], 1, &ssaoKernel[i][0]);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

	m_gBuffer->release();
}

void SSAOGLWidget::renderSSAOBlur()
{
	makeCurrent();

	m_gBuffer->bind();
	GLenum bufs[] = { GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, bufs);

	glClearColor(m_bkgColor.red() / 255.0f, m_bkgColor.green() / 255.0f, m_bkgColor.blue() / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_SSAOBlurProgram.m_program->bind();
	glBindVertexArray(m_quadVAO);
	QVector<uint> texIDs = m_gBuffer->textures();

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, texIDs[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glUniform1i(m_SSAOBlurProgram.m_ssaoTextureLoc, 4);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_gBuffer->release();
}

void SSAOGLWidget::renderLight()
{
	makeCurrent();

	// Next, we have to paint the textures obtained into the quad!!		
	m_basciProgram.m_program->bind();
	glBindVertexArray(m_quadVAO);

	QVector<uint> texIDs = m_gBuffer->textures();


	// Phong
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, texIDs[2]);
	glUniform1i(m_basciProgram.m_phongTexLoc, 6);

	// Ambient occlusion
	glUniform1i(m_basciProgram.m_basicLoc, m_renderResult);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, texIDs[4]);
	glUniform1i(m_basciProgram.m_ssaoTexLoc, 5);

	//gPos
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texIDs[0]);
	glUniform1i(m_basciProgram.m_ssaogPositionLoc, 1);
	//
	//Normal
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texIDs[1]);
	glUniform1i(m_basciProgram.m_ssaoNormalLoc, 2);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void SSAOGLWidget::setLighting()
{
	// Light source attached to the camera
	m_GProgram.m_program->bind();
	m_lightPos = glm::vec3(1.0f, 1.0f, 0.0f);
	glUniform3fv(m_GProgram.m_lightPosLoc, 1, &m_lightPos[0]);

	m_lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
	glUniform3fv(m_GProgram.m_lightColLoc, 1, &m_lightCol[0]);
}


void SSAOGLWidget::setFPSCameraSpeed(float val)
{
	m_moveSpeed = val;
}float SSAOGLWidget::getFPSCameraSpeed()const
{
	return m_moveSpeed;
}

void SSAOGLWidget::selectCameraType(CameraType index)
{
	m_cameraType = index;
}CameraType SSAOGLWidget::getCameraType()const
{
	return m_cameraType;
}

void SSAOGLWidget::setZNear(float value)
{
	m_zNear = value;
	projectionTransform();
	update();
}
float SSAOGLWidget::getZNear()const
{
	return m_zNear;
}

void SSAOGLWidget::setZFar(float value)
{
	m_zFar = value;
	projectionTransform();
	update();
}
float SSAOGLWidget::getZFar()const
{
	return m_zFar;
}

void SSAOGLWidget::setRenderResult(RenderResult val)
{
	m_renderResult = val;
}
RenderResult SSAOGLWidget::getRenderResult()const
{
	return m_renderResult;
}