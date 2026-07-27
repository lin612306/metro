QT       += core gui widgets serialport charts

TARGET = IntegratedSystem
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainframe.cpp \
    chartdialog.cpp \
    serialmanager.cpp \
    motorwidget.cpp \
    tempwidget.cpp \
    co2widget.cpp

HEADERS += \
    mainframe.h \
    chartdialog.h \
    serialmanager.h \
    motorwidget.h \
    tempwidget.h \
    co2widget.h
