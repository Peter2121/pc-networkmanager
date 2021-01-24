TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        addr.cpp \
        address_ip4.cpp \
        address_ip6.cpp \
        address_link.cpp \
        dummy_worker.cpp \
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
    addr.h \
    address_base.h \
    address_ip4.h \
    address_ip6.h \
    address_link.h \
    dummy_worker.h \
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
