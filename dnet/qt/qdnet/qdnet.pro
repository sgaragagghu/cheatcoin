#-------------------------------------------------
#
# Project created by QtCreator 2017-03-24T20:56:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qdnet
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += QDNET

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    ../../dnet_stream.c \
    ../../dnet_crypt.c \
    ../../dnet_database.c \
    ../../dnet_threads.c \
    ../../dnet_packet.c \
    ../../dnet_main.c \
    ../../dnet_command.c \
    ../../dnet_connection.c \
    ../../dnet_log.c \
    ../../../dus/programs/dar/source/lib/crc_c.c \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_crypt.c \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_random.c \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_string.c \
    ../../../dus/programs/dfstools/source/lib/dfsrsa.c \
	../../dnet_files.c \
	../../dnet_tap.c

HEADERS  += mainwindow.h \
    ../../dnet_history.h \
    ../../dnet_database.h \
    ../../dnet_crypt.h \
    ../../dnet_stream.h \
    ../../dnet_packet.h \
    ../../dnet_log.h \
    ../../dnet_threads.h \
    ../../dnet_command.h \
    ../../dnet_connection.h \
    ../../dnet_main.h \
    ../../../dus/programs/dar/source/include/crc.h \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_crypt.h \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_random.h \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_string.h \
    ../../../dus/programs/dfstools/source/dfslib/dfslib_types.h \
    ../../../dus/programs/dfstools/source/include/dfsrsa.h \
	../../dnet_files.h \
	../../dnet_tap.h \
	../../system.h

FORMS    += mainwindow.ui

CONFIG += mobility
MOBILITY = 

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

