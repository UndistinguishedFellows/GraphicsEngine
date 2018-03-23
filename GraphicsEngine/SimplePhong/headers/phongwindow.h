#ifndef PHONGWINDOW_H
#define PHONGWINDOW_H

#include <QWidget>
#include "ui_basicwindow.h"

class MainWindow;
class PhongGLWidget;

class PhongWindow : public QWidget
{
	Q_OBJECT

public:
	PhongWindow(MainWindow* mw);
	~PhongWindow();

private slots:
	void dockUndock();

private:
	Ui::BasicWindow m_ui;
	MainWindow* m_mainWindow;
	BasicGLWidget* m_glWidget;
};

#endif // !PHONGWINDOW_H