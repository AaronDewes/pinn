QT += gui waylandcompositor

HEADERS += \
    window.h \
    compositor.h

SOURCES += main.cpp \
    window.cpp \
    compositor.cpp


RESOURCES += qwindow-compositor.qrc

TARGET = qwindow-compositor
INSTALLS += target
