#ifndef ABSTRACTWINDOW_H
#define ABSTRACTWINDOW_H

#include <QWidget>

class MainWindow;

class AbstractWindow : public QWidget
{
	Q_OBJECT

public:
	AbstractWindow(MainWindow *mw) { mainWindow = mw; }
	virtual ~AbstractWindow(){}

protected:
	virtual void keyPressEvent(QKeyEvent *event) override{}

public slots:
	virtual void dockUndock() = 0;

protected:
	MainWindow *mainWindow;
};

#endif