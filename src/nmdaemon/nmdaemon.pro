TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        address_ip4.cpp \
        address_ip6.cpp \
        dummy_worker.cpp \
        ipaddr.cpp \
        loguru.cpp \
        main.cpp \
        nmcommand_data.cpp \
        nmdaemon.cpp \
        system_worker.cpp

LIBS += $$PWD/../../../../../sockpp/build-debug/libsockpp.a -lpthread -ldl
#LIBS += -L$$PWD/../../../../../sockpp/build-debug -lsockpp -lpthread -ldl

INCLUDEPATH += $$PWD/../../../../../sockpp/include
DEPENDPATH += $$PWD/../../../../../sockpp/include

INCLUDEPATH += $$PWD/../../../../../sockpp/build/generated
DEPENDPATH += $$PWD/../../../../../sockpp/build/generated

PRE_TARGETDEPS += $$PWD/../../../../../sockpp/build-debug/libsockpp.a

HEADERS += \
    address.h \
    address_ip4.h \
    address_ip6.h \
    dummy_worker.h \
    ipaddr.h \
    json.hpp \
    loguru.hpp \
    magic_enum.hpp \
    nmcommand.h \
    nmcommand_data.h \
    nmdaemon.h \
    nmjsonconst.h \
    nmworker.h \
    system_worker.h \
    workers.h
