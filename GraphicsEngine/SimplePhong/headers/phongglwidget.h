#ifndef PHONGGLWIDGET_H
#define PHONGGLWIDGET_H

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



class PhongGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    PhongGLWidget(QWidget *parent = 0);
    ~PhongGLWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

	void SelectCameraType(CameraType type);

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
    // Shaders
	void loadShaders();
	void reloadShaders();

	// Camera
	void projectionTransform(); // Type of camera
	void resetCamera();
	void viewTransform(); // Position of the camera

	// Scene
	void changeBackgroundColor();
	void createBuffersScene();
	void computeBBoxScene();
	void sceneTransform(); // Position and orientation of the scene

	// FPS
	void computeFps();
	void showFps();

	// Light info
	void SendPhongInfo();
	
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

	// Scene
	glm::vec3 m_sceneCenter;
	float m_sceneRadius;
	GLuint m_VAO, m_VBOVerts, m_VBONorms, m_VBOCols;
	QColor m_bkgColor;
	bool m_backFaceCulling;
	InteractiveAction m_Interaction;

	// Camera type
	CameraType m_cameraType;

	// Mouse
	int m_xClick;
	int m_yClick;
	float m_xRot;
	float m_yRot;

	// Phong 
	//		Light
	glm::vec3 m_lightPos;
	glm::vec3 m_lightCol;
	GLuint m_lightPosLoc, m_lightColorLoc, m_matAmbLoc, m_matDifLoc, m_matSpecLoc, m_matShinLoc;

	// Shaders
    QOpenGLShaderProgram *m_program;
	GLuint m_transLoc, m_projLoc, m_viewLoc;
	GLuint m_vertexLoc, m_normalLoc, m_colorLoc;

	// FPS
	bool m_showFps;
	QTime m_timer;
	uint m_frameCount;
	uint m_fps;
	
};

#endif
