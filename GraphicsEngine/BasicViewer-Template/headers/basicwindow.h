#include <QWidget>
#include "ui_basicwindow.h"

class MainWindow;
class BasicGLWidget;

class BasicWindow : public QWidget
{
	Q_OBJECT

public:
	BasicWindow(MainWindow* mw);
	~BasicWindow();

private slots:
	void dockUndock();
	void SelectCameraType(int index);

private:
	Ui::BasicWindow m_ui;
	MainWindow* m_mainWindow;
	BasicGLWidget* m_glWidget;
};
