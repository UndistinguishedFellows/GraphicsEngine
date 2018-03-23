TEMPLATE = app

HEADERS       = glwidget.h \
                window.h \
                mainwindow.h \
                logo.h \
                definitions.h \
                BasicViewer-Template/headers/basicglwidget.h \
                BasicViewer-Template/headers/basicwindow.h \
                #SimplePhong/headers/phongglwidget.h \
                #SimplePhong/headers/phongwindow.h \

SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp \
                mainwindow.cpp \
                logo.cpp \
                BasicViewer-Template/sources/basicglwidget.cpp \
                BasicViewer-Template/sources/basicwindow.cpp \
                #SimplePhong/sources/phongglwidget.cpp \
                #SimplePhong/sources/phongwindow.cpp \


FORMS		  = BasicViewer-Template/forms/basicwindow.ui \
				#SimplePhong/forms/phongwindow.ui \

QT           += widgets \
				opengl

CONFIG		 += console

INCLUDEPATH  += ThirdParty/glm \
		BasicViewer-Template/headers \
		#SimplePhong/headers \

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/GraphicsEngine
INSTALLS += target
