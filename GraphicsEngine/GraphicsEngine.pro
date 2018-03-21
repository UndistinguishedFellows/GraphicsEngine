TEMPLATE = app

HEADERS       = glwidget.h \
                window.h \
                mainwindow.h \
                logo.h \
                BasicViewer-Template/headers/basicglwidget.h \
                BasicViewer-Template/headers/basicwindow.h \
                BasicViewer-Template/headers/phongglwidget.h \
                BasicViewer-Template/headers/phongwindow.h \

SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp \
                mainwindow.cpp \
                logo.cpp \
                BasicViewer-Template/sources/basicglwidget.cpp \
                BasicViewer-Template/sources/basicwindow.cpp \
                SimplePhong/sources/phongglwidget.cpp \
                SimplePhong/sources/phongwindow.cpp \


FORMS		  = BasicViewer-Template/forms/basicwindow.ui \
				SimplePhong/forms/phongwindow.ui \

QT           += widgets \
				opengl

CONFIG		 += console

INCLUDEPATH  += glm \
				SimpleViewer-Template/headers \
				SimplePhong/headers \

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/GraphicsEngine
INSTALLS += target
