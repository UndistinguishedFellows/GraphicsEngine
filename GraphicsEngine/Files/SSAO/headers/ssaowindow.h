#include <QWidget>
#include "ui_ssaowindow.h"

class MainWindow;
class SSAOGLWidget;

class SSAOWindow : public QWidget
{
	Q_OBJECT

public:
	SSAOWindow(MainWindow* mw);
	~SSAOWindow();

private slots:
	void dockUndock();
	void selectCameraType(int index);
	void cameraSpeedChanged(double value);
	void znearChanged(double value);
	void zfarChanged(double value);
	void renderResultChanged(int index);

private:
	Ui::SSSSAOWindow m_ui;
	MainWindow* m_mainWindow;
	SSAOGLWidget* m_glWidget;
};
