#Graphics Engine

TEMPLATE = app
QT += core gui widgets opengl
CONFIG += console

INCLUDEPATH += Files \
				Files/ThirdParty \
				Files/SSAO/headers \
				Files/RT/headers \

include(GraphicsEngine.pri)

#install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/GraphicsEngine
INSTALLS += target