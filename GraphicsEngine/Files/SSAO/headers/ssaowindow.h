#include <QWidget>
#include "ui_ssaowindow.h"
#include "AbstractWindow.h"

class MainWindow;
class SSAOGLWidget;

class SSAOWindow : public AbstractWindow
{
	Q_OBJECT

public:
	SSAOWindow(MainWindow* mw);
	~SSAOWindow();

private slots:
	void dockUndock() override;
	void selectCameraType(int index);
	void cameraSpeedChanged(double value);
	void znearChanged(double value);
	void zfarChanged(double value);
	void renderResultChanged(int index);

private:
	Ui::SSSSAOWindow m_ui;
	SSAOGLWidget* m_glWidget;
};
