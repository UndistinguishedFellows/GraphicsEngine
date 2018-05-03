#include "../headers/ssaoglwidget.h"
#include "../headers/ssaowindow.h"
#include "mainwindow.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include "definitions.h"

SSAOWindow::SSAOWindow(MainWindow* mw) :m_mainWindow(mw)
{
	m_ui.setupUi(this);

	// Insert the m_glWidget in the GUI
	m_glWidget = new SSAOGLWidget("./Files/SSAO/models/sponza.obj", false, this);
	QVBoxLayout* layoutFrame = new QVBoxLayout(m_ui.qGLFrame);
	layoutFrame->setMargin(0);
	layoutFrame->addWidget(m_glWidget);
	m_glWidget->show();
	
	connect(m_ui.qUndockButton, SIGNAL(clicked()), this, SLOT(dockUndock()));
	
	// Connect the camera type
	m_ui.cameraType_combo->addItem(QString("Static"));
	m_ui.cameraType_combo->addItem(QString("FPS"));
	m_ui.cameraType_combo->setCurrentIndex((int)m_glWidget->getCameraType());
	connect(m_ui.cameraType_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectCameraType(int)));

	// Connect the camera speed
	m_ui.fpsCamSpeedSpinBox->setValue(m_glWidget->getFPSCameraSpeed());
	connect(m_ui.fpsCamSpeedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(cameraSpeedChanged(double)));

	// Connect the z near 
	m_ui.znearSpinBox->setValue(m_glWidget->getZNear());
	connect(m_ui.znearSpinBox, SIGNAL(valueChanged(double)), this, SLOT(znearChanged(double)));

	// Connect the z far
	m_ui.zfarSpinBox->setValue(m_glWidget->getZFar());
	connect(m_ui.zfarSpinBox, SIGNAL(valueChanged(double)), this, SLOT(zfarChanged(double)));

	// Connect the render result combo
	m_ui.renderResultComboBox->addItem(QString("Final result"));
	m_ui.renderResultComboBox->addItem(QString("G position"));
	m_ui.renderResultComboBox->addItem(QString("Normal"));
	m_ui.renderResultComboBox->addItem(QString("Ambient oclussion"));
	m_ui.renderResultComboBox->addItem(QString("Color"));
	m_ui.renderResultComboBox->setCurrentIndex(0);
	connect(m_ui.renderResultComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(renderResultChanged(int)));
}

SSAOWindow::~SSAOWindow()
{
	if (m_glWidget != nullptr) {
		delete m_glWidget;
		m_glWidget = nullptr;
	}
		
}

void SSAOWindow::dockUndock()
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

void SSAOWindow::selectCameraType(int index)
{
	if (m_glWidget) m_glWidget->selectCameraType((CameraType)index);
}

void SSAOWindow::cameraSpeedChanged(double value)
{
	if (m_glWidget) m_glWidget->setFPSCameraSpeed(value);
}

void SSAOWindow::znearChanged(double value)
{
	if (m_glWidget) m_glWidget->setZNear(value);
}

void SSAOWindow::zfarChanged(double value)
{
	if (m_glWidget) m_glWidget->setZFar(value);
}

void SSAOWindow::renderResultChanged(int index)
{
	if (m_glWidget) m_glWidget->setRenderResult((RenderResult)index);
}
