TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 #-stdlib=libc++

INCLUDEPATH += /home/tigran/data/prog.projects/work/include

LIBS += -lpthread
#LIBS += -lc++ -lpthread

SOURCES += main.cpp
