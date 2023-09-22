TEMPLATE	= app
LANGUAGE	= C++
QT += core gui widgets network concurrent
QT += testlib

CONFIG	+= qt warn_on release
CONFIG += qtc_runnable
CONFIG += c++17

LIBS	+= -L/usr/local/lib -ltrueos-utils -ltrueos-ui
LIBS    += /usr/local/lib/libsockpp.a

HEADERS	+= NetworkTray.h ../wificonfig/netKey.h
HEADERS	+= ../ethernetconfig/ethernetconfig.h ../ethernetconfig/ethernetconfigdata.h ../ethernetconfig/ipv4addrinput.h
HEADERS	+= ../wificonfig/dialogwpaenterprise.h ../wificonfig/dialogwpapersonal.h ../wificonfig/wepconfig.h
HEADERS	+= ../wificonfig/wificonfigwidgetbase.h ../wificonfig/wifiscanssid.h ../wificonfig/wifiselectiondialog.h
HEADERS	+= ../wificonfig/wpasuppnetwork.h ../wificonfig/wifiedit.h ../wificonfig/qlistwidgetitemnet.h


SOURCES	+= main.cpp NetworkTray.cpp ../wificonfig/netKey.cpp
SOURCES	+= ../ethernetconfig/ethernetconfig.cpp ../ethernetconfig/ethernetconfigdata.cpp ../ethernetconfig/ipv4addrinput.cpp
SOURCES	+= ../wificonfig/dialogwpaenterprise.cpp ../wificonfig/dialogwpapersonal.cpp ../wificonfig/wepconfig.cpp
SOURCES	+= ../wificonfig/wificonfigwidgetbase.cpp ../wificonfig/wifiscanssid.cpp ../wificonfig/wifiselectiondialog.cpp
SOURCES	+= ../wificonfig/wpasuppnetwork.cpp ../wificonfig/wifiedit.cpp ../wificonfig/qlistwidgetitemnet.cpp

#Include the backend classes for probing network info
include("../backend/backend.pri")
include("../NMDaemonClient/NMDaemonClient.pri")
include("../Utils/Utils.pri")

FORMS	+= ../wificonfig/netKey.ui
FORMS	+= ../ethernetconfig/ethernetconfig.ui ../ethernetconfig/ipv4addrinput.ui
FORMS   += ../wificonfig/dialogwpaenterprise.ui ../wificonfig/dialogwpapersonal.ui ../wificonfig/wepconfig.ui
FORMS   += ../wificonfig/wificonfigwidgetbase.ui ../wificonfig/wifiscanssid.ui ../wificonfig/wifiselectiondialog.ui
FORMS   += ../wificonfig/wifiedit.ui


RESOURCES += ../ethernetconfig/ethernetconfig.qrc
RESOURCES += ../wificonfig/wificonfig.qrc

TRANSLATIONS =  i18n/NetworkTray_af.ts \
		i18n/NetworkTray_ar.ts \
		i18n/NetworkTray_az.ts \
		i18n/NetworkTray_bg.ts \
		i18n/NetworkTray_bn.ts \
		i18n/NetworkTray_bs.ts \
		i18n/NetworkTray_ca.ts \
		i18n/NetworkTray_cs.ts \
		i18n/NetworkTray_cy.ts \
		i18n/NetworkTray_da.ts \
		i18n/NetworkTray_de.ts \
		i18n/NetworkTray_el.ts \
		i18n/NetworkTray_en_GB.ts \
		i18n/NetworkTray_en_ZA.ts \
		i18n/NetworkTray_es.ts \
		i18n/NetworkTray_et.ts \
		i18n/NetworkTray_eu.ts \
		i18n/NetworkTray_fa.ts \
		i18n/NetworkTray_fi.ts \
		i18n/NetworkTray_fr.ts \
		i18n/NetworkTray_fr_CA.ts \
		i18n/NetworkTray_gl.ts \
		i18n/NetworkTray_he.ts \
		i18n/NetworkTray_hi.ts \
		i18n/NetworkTray_hr.ts \
		i18n/NetworkTray_hu.ts \
		i18n/NetworkTray_id.ts \
		i18n/NetworkTray_is.ts \
		i18n/NetworkTray_it.ts \
		i18n/NetworkTray_ja.ts \
		i18n/NetworkTray_ka.ts \
		i18n/NetworkTray_ko.ts \
		i18n/NetworkTray_lt.ts \
		i18n/NetworkTray_lv.ts \
		i18n/NetworkTray_mk.ts \
		i18n/NetworkTray_mn.ts \
		i18n/NetworkTray_ms.ts \
		i18n/NetworkTray_mt.ts \
		i18n/NetworkTray_nb.ts \
		i18n/NetworkTray_nl.ts \
		i18n/NetworkTray_pa.ts \
		i18n/NetworkTray_pl.ts \
		i18n/NetworkTray_pt.ts \
		i18n/NetworkTray_pt_BR.ts \
		i18n/NetworkTray_ro.ts \
		i18n/NetworkTray_ru.ts \
		i18n/NetworkTray_sa.ts \
		i18n/NetworkTray_sk.ts \
		i18n/NetworkTray_sl.ts \
		i18n/NetworkTray_sr.ts \
		i18n/NetworkTray_sv.ts \
		i18n/NetworkTray_sw.ts \
		i18n/NetworkTray_ta.ts \
		i18n/NetworkTray_tg.ts \
		i18n/NetworkTray_th.ts \
		i18n/NetworkTray_tr.ts \
		i18n/NetworkTray_uk.ts \
		i18n/NetworkTray_ur.ts \
		i18n/NetworkTray_uz.ts \
		i18n/NetworkTray_vi.ts \
		i18n/NetworkTray_zh_CN.ts \
		i18n/NetworkTray_zh_TW.ts \
		i18n/NetworkTray_zu.ts

INSTALLS += target

QMAKE_LIBDIR = /usr/local/lib /usr/local/lib/qt5 $$PWD/../../../trueos-libqt5/src-qt5/libtrueos/

TARGET  = pc-nettray
target.path = /usr/local/bin/

INCLUDEPATH += /usr/local/include $$PWD/../../../trueos-libqt5/src-qt5/libtrueos/ui $$PWD/../../../trueos-libqt5/src-qt5/libtrueos/utils
INCLUDEPATH += $$PWD/../wificonfig
INCLUDEPATH += $$PWD/../ethernetconfig

