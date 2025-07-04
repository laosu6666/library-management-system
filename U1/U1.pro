QT       += core gui
QT        +=core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addbookdialog.cpp \
    book.cpp \
    comment.cpp \
    creditdialog.cpp \
    database.cpp \
    library.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    user.cpp \
    usermanagerdialog.cpp

HEADERS += \
    addbookdialog.h \
    book.h \
    comment.h \
    creditdialog.h \
    database.h \
    library.h \
    login.h \
    mainwindows.h \
    user.h \
    usermanagerdialog.h
QT      += sql

FORMS += \
    addbookdialog.ui \
    creditdialog.ui \
    login.ui \
    mainwindow.ui \
    usermanagerdialog.ui
INCLUDEPATH += $$OUT_PWD

TRANSLATIONS += \
    U1_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    C:/Users/HUAWEI/Downloads/library-management-system-Branch1/library-management-system-Branch1/U1/resources.qrc



