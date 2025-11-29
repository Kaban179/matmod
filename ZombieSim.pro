QT       += core gui widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \
    worldobject.cpp

HEADERS += \
    mainwindow.h \
    qcustomplot.h \
    worldobject.h

TARGET = ZombieSim
