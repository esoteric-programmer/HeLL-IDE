#-------------------------------------------------
#
# Project created by QtCreator 2014-10-31T13:22:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = hell-ide

SOURCES += main.cpp\
        mainwindow.cpp \
    qscilexerhell.cpp \
    qconsolewidget.cpp \
    qioworker.cpp \
    qioprocessworker.cpp \
    qmalbolgerunner.cpp \
    qsleep.cpp \
    preferencesdialog.cpp \
    find.cpp \
    replace.cpp \
    qabstractmalbolgerunner.cpp \
    qmalbolgedebugger.cpp \
    lmaodebuginformations.cpp \
    memorycellparser.cpp

HEADERS  += mainwindow.h \
    qscilexerhell.h \
    qconsolewidget.h \
    qioworker.h \
    qioprocessworker.h \
    qmalbolgerunner.h \
    qsleep.h \
    preferencesdialog.h \
    properties.h \
    find.h \
    replace.h \
    qabstractmalbolgerunner.h \
    qmalbolgedebugger.h \
    lmaodebuginformations.h \
    malbolgedefinitions.h \
    memorycellparser.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui \
    find.ui \
    replace.ui

greaterThan(QT_MAJOR_VERSION, 4) {
    win32 {
        LIBS += -Lqscintilla/Qt4Qt5 -lqscintilla2
    }
    !win32 {
        LIBS += -lqscintilla2_qt5
    }
}
lessThan(QT_MAJOR_VERSION, 5) {
    LIBS += -Lqscintilla/Qt4 -lqscintilla2_qt4
}
