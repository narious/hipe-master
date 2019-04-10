#-------------------------------------------------
#
# Project created by QtCreator 2015-09-04T14:33:09
#
#-------------------------------------------------

QT       += core gui
QT       += webkit
QT       += webkitwidgets
QT       += printsupport
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hiped
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    keylist.cpp \
    connection.cpp \
    connectionmanager.cpp \
    container.cpp \
    containerframe.cpp \
    containertoplevel.cpp \
    hipe_instruction.c \
    common.c \
    sanitation.cpp

HEADERS += main.hpp \
    ExpArray.hh \
    keylist.h \
    common.h \
    connection.h \
    connectionmanager.h \
    container.h \
    containerframe.h \
    containertoplevel.h \
    hipe_instruction.h \
    sanitation.h

QMAKE_CXXFLAGS += -std=c++11 -Ofast -pthread
LIBS += -pthread
