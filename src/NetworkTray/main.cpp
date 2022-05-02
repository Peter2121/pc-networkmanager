 
#include <err.h>

#include <qtimer.h>
#include <QTranslator>
#include <QDebug>
#include <QFile>
#include <qtextcodec.h>
#include <QApplication>
#include <QLocale>
#include <QTextCodec>
#include <QSignalSpy>

#include <NetworkTray.h>
#include <unistd.h>

#include "../NMDaemonClient/nmdaemoncheckstatus.h"
#include "../NMDaemonClient/nmdaemonspy.h"
#include "../backend/backend-network.h"

#define PREFIX QString("/usr/local")

bool PICOSESSION = false;

int  main(int argc, char *argv[]) {

//  PICOSESSION = !QString(getenv("PICO_CLIENT_LOGIN")).isEmpty();

//  QString dev = (PICOSESSION ? "PICOSESSION" : "");
    QString dev, dev_status, str_value;
    QStringList devs;
    QJsonValue json_value;
    QJsonObject json_object;
    QJsonArray json_array;
    NMDaemonCheckStatus* nmd_status;
//  if(argc==1 && !PICOSESSION){
    QApplication a(argc, argv);
    nmd_status = new NMDaemonCheckStatus();
    NMDaemonSpy* spy = new NMDaemonSpy(nmd_status, &NMDaemonCheckStatus::ifStatusUpdated);
    nmd_status->startMonitoring();
    if(!spy->wait(5000))
        qDebug() << "Signal NMDaemonCheckStatus::ifStatusUpdated was not received in time";
    delete spy;
    QJsonArray if_status = nmd_status->getLastIfStatus();
    if(!if_status.empty())
    {
//        json_value = if_status.value("ADDRESSES");
//        json_array = json_value.toArray();
        foreach (const QJsonValue &v, if_status)
        {
            json_object = v.toObject();
            json_value = json_object.value("STATUS");
            str_value = json_value.toString();
            if( (str_value!="ACTIVE") && (str_value!="ASSOCIATED") && (str_value!="NO_CARRIER"))
                continue;
            json_value = json_object.value("MEDIA");
            str_value = json_value.toString();
            if(!str_value.contains("Ethernet") && !str_value.contains("Wireless Ethernet"))
                continue;
            json_value = json_object.value("INTERFACE NAME");
            dev = json_value.toString();
            devs.push_back(dev);
        }
    }
    QStringList wifi = devs.filter("wlan");
    qDebug() << "Found Devices:" << devs << wifi;
    //no device specified - automatically figure out which one(s) to use
//   QStringList devs = backend::NetDevice::listNetDevices();
//   QStringList wifi = devs.filter("wlan");
//    qDebug() << "Found Devices:" << devs << wifi;
//    wifi.sort();
    //Look for wifi devices first
/*
    for(int i=0; i<wifi.length(); i++){
      devs.removeAll(wifi[i]); //already handled
      if(i==0){ dev = wifi[i]; } //use the first wifi device
//      else if(!PICOSESSION){
      else {
        //multiple wifi devices - open more tray devices
        QProcess::startDetached("pc-nettray "+devs[i]);
      }
    }
    //Now look for wired devices
    for(int i=0; i<devs.length() && dev.isEmpty(); i++){
      if(devs[i].startsWith("lo")){ continue; } //don't use loopback devices
      dev = devs[i];
    }
  qDebug() << "Using device:" << dev;
*/

   bool ready = false;
   for(int i=0; i<60 && !ready; i++){
      ready = QSystemTrayIcon::isSystemTrayAvailable();
      if(!ready){
	//Pause for 5 seconds
        sleep(5); //don't worry about stopping event handling - nothing running yet
      }
   }
   if(!ready){
     qDebug() << "Could not find any available system tray after 5 minutes: exiting....";
     return 1;
   }
   
   QTranslator translator;
   QLocale mylocale;
   QString langCode = mylocale.name();
   if ( ! QFile::exists( PREFIX + "/share/trueos/i18n/NetworkTray_" + langCode + ".qm" ) )
     langCode.truncate(langCode.indexOf("_"));
   translator.load( QString("NetworkTray_") + langCode, PREFIX + "/share/trueos/i18n/" );
   a.installTranslator( &translator );
   qDebug() << "Locale:" << langCode;
   QTextCodec::setCodecForLocale( QTextCodec::codecForName("UTF-8") ); //Force Utf-8 compliance
   QApplication::setQuitOnLastWindowClosed(false);
   NetworkTray *tray = nullptr;

   for(auto d : devs)
   {
       qDebug() << "Starting NetworkTray for device " << d;
       tray = new NetworkTray(nmd_status);
       tray->programInit(d);
   }

   return a.exec();
}
