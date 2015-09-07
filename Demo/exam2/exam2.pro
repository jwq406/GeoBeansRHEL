#-------------------------------------------------
#
# Project created by QtCreator 2015-08-12T09:56:57
#
#-------------------------------------------------

QT       += core gui xml network opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


greaterThan(QT_MAJOR_VERSION, 4){
CONFIG += c++11
} else {
QMAKE_CXXFLAGS += -std=c++0x
}

CONFIG += qt debug warn_on
CONFIG += qtestlib

TARGET = exam2
TEMPLATE = app
LIBS += -L/usr/local/lib -lqgis_core -lqgis_gui -lqgis_app -L/usr/local/lib64 -losgEarth -losgEarthUtil -losgEarthQt -losgDB -losgViewer
INCLUDEPATH += /usr/local/include/qgis /usr/local/include /usr/local/include
DEFINES += GUI_EXPORT= CORE_EXPORT= APP_EXPORT=  override=


SOURCES += main.cpp\
    examp2.cpp \
    qgsmaptooldrawline.cpp \
    qgsmaptoolselect.cpp \
    qgsutility.cpp \
    flycontrolsetting.cpp \
    mymainwindow.cpp

HEADERS  += \
    examp2.h \
    qgsmaptooldrawline.h \
    qgsmaptoolselect.h \
    qgsutility.h \
    flycontrolsetting.h \
    mymainwindow.h

FORMS    += \
    examp2.ui \
    flycontrolsetting.ui \
    mymainwindow.ui

RESOURCES += \
    examp2.qrc \
    mymainwindowres.qrc
