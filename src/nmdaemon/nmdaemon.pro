TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        dummy_worker.cpp \
        loguru.cpp \
        main.cpp \
        nmcommand_data.cpp \
        nmdaemon.cpp

LIBS += $$PWD/../../../../../sockpp/build-debug/libsockpp.a -lpthread -ldl
#LIBS += -L$$PWD/../../../../../sockpp/build-debug -lsockpp -lpthread -ldl

INCLUDEPATH += $$PWD/../../../../../sockpp/include
DEPENDPATH += $$PWD/../../../../../sockpp/include

INCLUDEPATH += $$PWD/../../../../../sockpp/build/generated
DEPENDPATH += $$PWD/../../../../../sockpp/build/generated

PRE_TARGETDEPS += $$PWD/../../../../../sockpp/build-debug/libsockpp.a

HEADERS += \
    dummy_worker.h \
    json.hpp \
    loguru.hpp \
    magic_enum.hpp \
    nmcommand.h \
    nmcommand_data.h \
    nmdaemon.h \
    nmjsonconst.h \
    nmworker.h \
    workers.h
