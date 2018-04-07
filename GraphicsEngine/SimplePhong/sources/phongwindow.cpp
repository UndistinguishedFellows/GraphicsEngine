#include "../SimplePhong/headers/phongwindow.h"
#include "../SimplePhong/headers/phongglwidget.h"
#include "mainwindow.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>

PhongWindow::PhongWindow(MainWindow* mw) :m_mainWindow(mw)
{
	m_ui.setupUi(this);

	// Insert the phongWidget in the GUI
	m_phongWidget = new PhongGLWidget(this);
	QVBoxLayout* layoutFrame = new QVBoxLayout(m_ui.qGLFrame);
	layoutFrame->setMargin(0);
	layoutFrame->addWidget(m_phongWidget);
	m_phongWidget->show();
	
	connect(m_ui.qUndockButton, SIGNAL(clicked()), this, SLOT(dockUndock()));
}

PhongWindow::~PhongWindow()
{
	if (m_phongWidget != nullptr) {
		delete m_phongWidget;
		m_phongWidget = nullptr;
	}
		
}

void PhongWindow::dockUndock()
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