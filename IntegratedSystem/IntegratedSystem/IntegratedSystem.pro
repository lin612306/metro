QT       += core gui widgets serialport charts

TARGET = IntegratedSystem
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainframe.cpp \
    chartdialog.cpp \
    motorwidget.cpp \
    tempwidget.cpp \
    co2widget.cpp

HEADERS += \
    mainframe.h \
    chartdialog.h \
    motorwidget.h \
    tempwidget.h \
    co2widget.h
