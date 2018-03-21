#include "../headers/basicglwidget.h"
#include "../headers/basicwindow.h"
#include "mainwindow.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>

BasicWindow::BasicWindow(MainWindow* mw) :m_mainWindow(mw)
{
	m_ui.setupUi(this);

	// Insert the m_glWidget in the GUI
	m_glWidget = new BasicGLWidget;
	QVBoxLayout* layoutFrame = new QVBoxLayout(m_ui.qGLFrame);
	layoutFrame->setMargin(0);
	layoutFrame->addWidget(m_glWidget);
	m_glWidget->show();
	
	connect(m_ui.qUndockButton, SIGNAL(clicked()), this, SLOT(dockUndock()));
}

BasicWindow::~BasicWindow()
{
	if (m_glWidget != nullptr) {
		delete m_glWidget;
		m_glWidget = nullptr;
	}
		
}

void BasicWindow::dockUndock()
{
	if (parent()) {
		setParent(0);
		setAttribute(Qt::WA_DeleteOnClose);
		move(QApplication::desktop()->width() / 2 - width() / 2,
			QApplication::desktop()->height() / 2 - height() / 2);
		m_ui.qUndockButton->setText(tr("Dock"));
		show();
	}
	else {
		if (!m_mainWindow->centralWidget()) {
			if (m_mainWindow->isVisible()) {
				setAttribute(Qt::WA_DeleteOnClose, false);
				m_ui.qUndockButton->setText(tr("Undock"));
				m_mainWindow->setCentralWidget(this);
				show();
			}
			else {
				QMessageBox::information(0, tr("Cannot dock"), tr("Main window already closed"));
			}
		}
		else {
			QMessageBox::information(0, tr("Cannot dock"), tr("Main window already occupied"));
		}
	}
}