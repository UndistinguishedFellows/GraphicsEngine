#ifndef SSAOGLWIDGET_H
#define SSAOGLWIDGET_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QColor>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QTime>
#include <QWheelEvent>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "definitions.h"
#include "Files/model.h"
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>
#include "definitions.h"

struct GProgram
{
public:
	QOpenGLShaderProgram * m_program;
	GLuint m_transLoc, m_projLoc, m_viewLoc;
	GLuint m_matAmbLoc, m_matDiffLoc, m_matSpecLoc, m_matShinLoc;
	GLuint m_VertexLoc, m_NormalLoc;
	GLuint m_lightPosLoc, m_lightColLoc;
};

struct SSAOProgram
{
public:
	QOpenGLShaderProgram * m_program;
	GLuint m_ssaoPosTexLoc, m_ssaoNTexLoc, m_ssaoBlurLoc, m_ssaoSampleKernelLoc[64];
};

struct SSAOBlurProgram
{
public:
	QOpenGLShaderProgram * m_program;
	GLuint m_wScreenLoc, m_hScreenLoc;
	GLuint m_ssaoTextureLoc;
};

struct BasicProgram
{
public:
	QOpenGLShaderProgram * m_program;
	GLuint m_basicLoc;
	GLuint m_phongTexLoc, m_ssaoTexLoc, m_ssaoNormalLoc, m_ssaogPositionLoc;

};

class SSAOGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT

public:
	SSAOGLWidget(QString modelFilename, bool showFps, QWidget *parent = 0);
	~SSAOGLWidget();

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	// Slot functions from AOWindow
	void setFPSCameraSpeed(float val);
	float getFPSCameraSpeed()const;

	void selectCameraType(CameraType index);
	CameraType getCameraType()const;

	void setZNear(float value);
	float getZNear()const;

	void setZFar(float value);
	float getZFar()const;

	void setRenderResult(RenderResult val);
	RenderResult getRenderResult()const;

	public slots:
	void cleanup();

signals:

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;

	// Keyboard and mouse interaction
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void printHelp();

	// Shaders
	void loadShaders();
	void loadGShader();
	void loadSSAOShader();
	void loadSSAOBlurShader();
	void loadBasicShader();
	void reloadShaders();

	// Camera
	void initCameraParams();
	void projectionTransform(); // Type of camera
	void resetCamera(bool sceneCenter = false);
	void viewTransform(); // Position of the camera

	// Scene
	void changeBackgroundColor();
	void computeCenterRadiusScene();

	void createQuad();

	void updateCameraVectors();

	// Model
	void loadModel();
	void cleanBuffersModel();
	void computeBBoxModel();
	void modelTransform(); // Position and orientation of the scene
	bool m_modelLoaded;

	//Lighting	
	void setLighting();

	// FPS
	void computeFps();
	void showFps();

	// Ambient occlusion
	void createFramebuffers();
	void createSampleKernel();
	void createNoiseTexture();

	std::vector<glm::vec3> ssaoKernel;
	unsigned int noiseTexture;

	/* Attributes */
	// Screen
	int m_width;
	int m_height;

	// Camera
	float m_ar;
	float m_fov;
	float m_fovIni;
	float m_zNear;
	float m_zFar;
	float m_radsZoom;
	float m_xPan;
	float m_yPan;
	// FPS camera
	glm::vec3 m_camPos; 
	glm::vec3 m_camFront;
	glm::vec3 m_camUp;
	glm::vec3 m_camRight;
	float m_Pitch; // Rotation on X
	float m_Yaw; // Rotation on Y

	float m_moveSpeed;
	float m_mouseSensitivity;
	bool m_constrainPitch;
	
	// Scene
	glm::vec3 m_sceneCenter;
	float m_sceneRadius;
	QColor m_bkgColor;
	bool m_backFaceCulling;

	// Model
	Model m_model;
	QString m_modelFilename;
	glm::vec3 m_modelCenter;
	float m_modelRadius;
	GLuint m_VAOModel, m_VBOModelVerts, m_VBOModelNorms;
	GLuint m_VBOModelMatAmb, m_VBOModelMatDiff, m_VBOModelMatSpec, m_VBOModelMatShin;

	// Lights
	glm::vec3 m_lightPos;
	glm::vec3 m_lightCol;

	// Mouse
	int m_xClick;
	int m_yClick;
	float m_xRot;
	float m_yRot;
	int m_doingInteractive;

	// Shaders
	GProgram m_GProgram;
	SSAOProgram m_SSAOProgram;
	SSAOBlurProgram m_SSAOBlurProgram;
	BasicProgram m_basciProgram;

	// FPS
	bool m_showFps;
	QTime m_timer;
	uint m_frameCount;
	uint m_fps;

	// Ambient Occlusion
	QOpenGLFramebufferObject *m_gBuffer;
	GLuint m_quadVAO;
	GLuint m_quadVBO;

	//Rendering
	void renderGBuffer();
	void renderSSAO();
	void renderSSAOBlur();
	void renderLight();

	CameraType m_cameraType = FPS;
	RenderResult m_renderResult = FINAL_RESULT;


};

#endif
