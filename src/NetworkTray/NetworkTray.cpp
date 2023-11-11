
/* Qt */
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QMenu>
#include <QProcess>
#include <QToolTip>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QWidgetAction>
#include <iostream>

#include "NetworkTray.h"
#include <trueos-netif.h>
#include <trueos-utils.h>

#define PREFIX QString("/usr/local")

// Public Variables
QString DeviceSSID;
QString DeviceSignalStrength = "";
int 	DeviceSavedStrength = 0;
QString DeviceWirelessSpeed = "";

QString username;

extern bool PICOSESSION;

NetworkTray::NetworkTray(NMDaemonCheckStatus* nmds) : QDialog()
{
     nmdStatus = nmds;
     QuitAction = nullptr;
     ConfigAction = nullptr;
     StatusAction = nullptr;
     TrayIcon = nullptr;
     TrayIconMenu = nullptr;
     LastWifiPower = 0;
     numWifiConnectRetries = 0;
     IsWireless = false;
     for(short i=0; i<NUM_SEPARATORS; i++)
     {
         SeparatorActions[i] = nullptr;
     }
     dialogEthernetConfig=std::make_unique<ethernetconfig>();
     EthConfig = nullptr;
     WlanConfig = nullptr;
}

NetworkTray::~NetworkTray()
{
    if(QuitAction != nullptr)
        delete QuitAction;
    if(ConfigAction != nullptr)
        delete ConfigAction;
    if(StatusAction != nullptr)
        delete StatusAction;
    foreach(const auto &pact, ConnectActions)
    {
        if(pact != nullptr)
            delete pact;
    }
    if(TrayIcon != nullptr)
        delete TrayIcon;
    if(TrayIconMenu != nullptr)
        delete TrayIconMenu;
    for(short i=0; i<NUM_SEPARATORS; i++)
    {
        if(SeparatorActions[i] != nullptr)
            delete SeparatorActions[i];
    }
    if(EthConfig!=nullptr)
        delete EthConfig;
}

void NetworkTray::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    qDebug() << "onActivated called with the reason: " << reason;
}

void NetworkTray::slotSave()
{
    nmdStatus->saveConfig(IfName);
}

void NetworkTray::programInit(QString device)
{
    //Setup all the null pointers to start with
    runCommandProc = 0;
    getIfProc = 0;
    trayActionGroup = 0;
    fileWatcherClosed = 0;
    DEVICE = 0;
    IfName = device;
    QAction* connect_action = nullptr;
    //
    // Load the icons we'll be using into memory
    // TODO:
    // 1. Use icons from theme if configured
    // 2. Move private icons to binary resource and load it
    //
    loadIcons();
    TrayIcon = new QSystemTrayIcon(this);

    QString tmp;
    QIcon status_icon;
    QString status;

    for(short i=0; i<NUM_SEPARATORS; i++)
    {
      SeparatorActions[i] = new QAction(this);
      SeparatorActions[i]->setSeparator(true);
    }

    QuitAction = new QAction(tr("&Quit"), this);
    QuitAction->setIcon(IconQuit);
    connect(QuitAction, &QAction::triggered, this, &NetworkTray::slotQuit);

    ConfigAction  = new QAction(tr("&Configure") + " " + IfName, this);
    ConfigAction->setIcon(IconConfigure);
    connect(ConfigAction, &QAction::triggered, this, &NetworkTray::openConfigDlg);

    if(device.startsWith(IF_WIFI_PREFIX))
    {
        IsWireless = true;

        ConfigWlanAction  = new QAction(tr("&Configure") + " Wi-Fi", this);
        ConfigWlanAction->setIcon(IconConfigure);
        connect(ConfigWlanAction, &QAction::triggered, this, &NetworkTray::openWlanConfigDlg);

        ResetAction  = new QAction(tr("&Reset Status"), this);
        ResetAction->setIcon(IconReset);
        connect(ResetAction, &QAction::triggered, this, &NetworkTray::resetWpaSupplicant);

        ScanAction  = new QAction(tr("&Scan Wifi"), this);
        ScanAction->setIcon(IconScan);
        connect(ScanAction, &QAction::triggered, this, &NetworkTray::scanWifi);

        SaveAction = new QAction(tr("Save"), this);
        SaveAction->setIcon(IconSave);
        connect(SaveAction, &QAction::triggered, this, &NetworkTray::slotSave);
    }

    TrayIconMenu = new QMenu(this);
    TrayIconMenu->addAction(ConfigAction);
    if(device.startsWith(IF_WIFI_PREFIX))
    {
        TrayIconMenu->addAction(ConfigWlanAction);
        TrayIconMenu->addAction(ResetAction);
        TrayIconMenu->addAction(ScanAction);
    }
    TrayIconMenu->addAction(SeparatorActions[0]);

  if(device.startsWith(IF_WIFI_PREFIX))
  {
    TrayIcon->setIcon(IconWifiDisconnect);
    QPair<QString,int> wifi_status = nmdStatus->getLastIfWifiNetwork(device);
    if(!wifi_status.first.isEmpty())
    {
        LastWifiNetwork = wifi_status.first;
        LastWifiPower = wifi_status.second;
        status = getWifiStatusLine(wifi_status, true);
        StatusAction  = new QAction(status, this);
        if(wifi_status.second>0)
        {
            TrayIcon->setIcon(getWifiIcon(wifi_status.second));
            StatusAction->setIcon(getWifiIcon(wifi_status.second));
        }
    }
    else
    {
        status = ">>> " + tr("disconnected") + " <<<";
        StatusAction  = new QAction(status, this);
    }
    connect(StatusAction, &QAction::triggered, this, &NetworkTray::slotShowIfStatus);
    TrayIconMenu->addAction(StatusAction);

    TrayIconMenu->addAction(SeparatorActions[1]);
    QVector<QPair<QString,int>> wifi_networks = nmdStatus->getLastNetsWifiPower(device);
    foreach(const auto &net, wifi_networks)
    {
        if( !wifi_status.first.isEmpty() && (net.first == wifi_status.first) )
            continue;
        status = getWifiStatusLine(net, false);
        connect_action = new QAction(status, this);
        if(net.second>0)
        {
            connect_action->setIcon(getWifiIcon(net.second));
        }
        connect(connect_action, &QAction::triggered, this, &NetworkTray::slotConnect);
        TrayIconMenu->addAction(connect_action);
        ConnectActions.append(connect_action);
    }
  }
  else
  {
    TrayIcon->setIcon(IconWiredConnect);
    StatusAction  = new QAction(IfName, this);
    StatusAction->setIcon(IconWiredConnect);
    connect(StatusAction, &QAction::triggered, this, &NetworkTray::slotShowIfStatus);
    TrayIconMenu->addAction(StatusAction);
  }

  TrayIconMenu->addAction(SeparatorActions[2]);
  //if(device.startsWith(IF_WIFI_PREFIX))
  // TrayIconMenu->addAction(SaveAction);
  TrayIconMenu->addAction(QuitAction);
  TrayIcon->setContextMenu(TrayIconMenu);
  TrayIcon->show();
/*
void QMenu::aboutToShow()
This signal is emitted just before the menu is shown to the user.
void QMenu::aboutToHide()
This signal is emitted just before the menu is hidden from the user.
*/
  connect(nmdStatus, &NMDaemonCheckStatus::wifiIfListUpdated, this, &NetworkTray::updateStatus);
  connect(nmdStatus, &NMDaemonCheckStatus::ifStatusUpdated, this, &NetworkTray::updateStatus);
  connect(nmdStatus, &NMDaemonCheckStatus::wifiNetStatusUpdated, this, &NetworkTray::updateStatus);
  connect(nmdStatus, &NMDaemonCheckStatus::wifiIfStatusUpdated, this, &NetworkTray::updateStatus);
  connect(nmdStatus, &NMDaemonCheckStatus::dataError, this, &NetworkTray::slotShowError);
  connect(TrayIconMenu, &QMenu::aboutToShow, this, &NetworkTray::setActiveScanTimeout);
  connect(TrayIconMenu, &QMenu::aboutToHide, this, &NetworkTray::setIdleScanTimeout);
//  connect(TrayIcon, &QSystemTrayIcon::activated, this, &NetworkTray::onActivated);
/*
  QObject::connect(TrayIconMenu, &QMenu::aboutToShow, [](){
      qDebug()<<"aboutToShow";
  });
  QObject::connect(TrayIconMenu, &QMenu::aboutToHide, [](){
      qDebug()<<"aboutToHide";
  });
*/
  LastUpdateTime = QDateTime::currentDateTime();
//  updateStatus();
//  monitorStatus();
}

QString NetworkTray::getWifiStatusLine(QPair<QString,int> wifi_status, bool connected)
{
    QString ret_status;
    QString prefix;
    QString suffix;

    if(connected)
    {
        prefix = ">>> ";
        suffix = " <<<";
    }

    if(wifi_status.second>0)
    {
        ret_status = prefix + wifi_status.first + " (" + QString(std::to_string(wifi_status.second).c_str()) + "%)" + suffix;
    }
    else
    {
        ret_status = prefix + wifi_status.first + suffix;
    }
    return ret_status;
}

void NetworkTray::slotConnect()
{
// TODO: check if psk is already present in WPA config file and ask user for the psk if needed
    if(numWifiConnectRetries>0)
        return;
    QString ssid = sender()->property("text").toString().split(" ").at(0);
    numWifiConnectRetries = 0;
    connectWifiNetwork(ssid);
}

void NetworkTray::connectWifiNetwork(QString ssid)
{
    if(numWifiConnectRetries>WIFI_CONNECT_MAX_RETRIES)
    {
        numWifiConnectRetries = 0;
        return;
    }
    if(numWifiConnectRetries>0)
        qDebug() << "Retrying connect to ssid " << ssid << " for the " << numWifiConnectRetries << "nd time";
    nmdStatus->stopMonitoring();
    connect(nmdStatus, SIGNAL(successWifiConnect(QString)), this, SLOT(slotWifiConnectSuccess(QString)));
    connect(nmdStatus, SIGNAL(errorWifiConnect(QString, QString)), this, SLOT(slotWifiConnectError(QString, QString)));
    nmdStatus->ConnectWifiNetwork(IfName, ssid);
    //qDebug() << "connectWifiNetwork to ssid " << ssid << " result: " << result;
    /*
//<3>Associated with 04:d1:3a:f4:53:68
//nmworkerwpa.cpp:385   INFO| wpaCtrlCmd received data: <3>Association request to the driver failed
//nmworkerieee80211.cpp:438    ERR| Error in execCmdIfStatus: cannot get bssid for interface wlan0 -> NO WLAN INTERFACE!!!
//nmworkerwpa.cpp:256    ERR| wpaCtrlCmd cannot connect to client socket /tmp/nmd_wpaw.xrEFI : Address already in use
//nmworkerieee80211.cpp:82     ERR| Error in execCmdNetList: incorrect data received from lib80211
//    nmdStatus->EnableNetwork(IfName, ssid);
    //nmdStatus->saveConfig(IfName);
    //updateStatus();
    nmdStatus->startMonitoring();
    */
}

void NetworkTray::slotWifiConnectError(QString ssid, QString result)
{
    if(result.toStdString() == JSON_DATA_ERR_NOT_AVAILABLE)
    {
        // TODO: Add translations
        QMessageBox::information(this, QString(JSON_DATA_ERR_NOT_AVAILABLE.c_str()), "The requested network " + ssid + " is not available");
        nmdStatus->startMonitoring();
    }
    else if( (result.toStdString() == JSON_PARAM_ERR) ||
             (result.toStdString() == JSON_DATA_ERR_AUTH_FAILED) )
    {
        dialogNetKey = new netKey();
        //Insert the SSID into the dialog
        dialogNetKey->setSSID(ssid);
        dialogNetKey->showKeyType(true);
        connect(dialogNetKey, SIGNAL( saved(QString,QString,bool,QString) ), this, SLOT( slotUpdatePskAndConnect(QString,QString,bool,QString) ) );
        connect(dialogNetKey, SIGNAL( cancel() ), nmdStatus, SLOT( slotStartMonitoring() ) );
        //Activate the dialog
        dialogNetKey->exec();
    }
    else if( result.toStdString() == JSON_PARAM_NOTFOUND )
    {
        dialogNetKey = new netKey();
        //Insert the SSID into the dialog
        dialogNetKey->setSSID(ssid);
        dialogNetKey->showKeyType(true);
        connect(dialogNetKey, SIGNAL( saved(QString,QString,bool,QString) ), this, SLOT( slotAddNetworkAndConnect(QString,QString,bool,QString) ) );
        connect(dialogNetKey, SIGNAL( cancel() ), nmdStatus, SLOT( slotStartMonitoring() ) );
        //Activate the dialog
        dialogNetKey->exec();
    }
    else if( result.toStdString() == JSON_DATA_ERR_DRV_REQ_FAILED )
    {
        nmdStatus->startMonitoring();
        QTimer::singleShot(WIFI_CONNECT_RETRY_MSEC, this, [=]() { connectWifiNetwork(ssid); } );
    }
}

void NetworkTray::slotWifiConnectSuccess(QString)
{
    disconnect(nmdStatus, SIGNAL(successWifiConnect(QString)), this, SLOT(slotWifiConnectSuccess(QString)));
    nmdStatus->updateStatus();
    nmdStatus->startMonitoring();
}

void NetworkTray::slotUpdatePskAndConnect(QString key, QString ssid, bool hexkey, QString security)
{
    nmdStatus->stopMonitoring();
    //if(nmdStatus->UpdatePsk(IfName, ssid, key, hexkey))
    if(nmdStatus->UpdatePskAndSave(IfName, ssid, key, hexkey))
    {
        connectWifiNetwork(ssid);
    }
    else
    {
        qDebug() << "Error updating PSK for network " << ssid;
    }
  //Run the wifiQuickConnect function
  //NetworkInterface::wifiQuickConnect(ssid,key,DEVICE->device(), hexkey);

  //Inform the user that it is connecting (this is done by libtrueos now)
  //QString msg = tr("Connecting to ") + SSID;
  //trayIcon->showMessage( tr("Please Wait"),msg,QSystemTrayIcon::NoIcon,10000);
    nmdStatus->startMonitoring();
}

void NetworkTray::slotAddNetworkAndConnect(QString key, QString ssid, bool hexkey)
{
    nmdStatus->stopMonitoring();
    connect(nmdStatus, &NMDaemonClient::successWifiNetAdd, this, &NetworkTray::slotWifiConnectNetwork);
    connect(nmdStatus, &NMDaemonClient::errorWifiNetAdd, this, &NetworkTray::slotWifiAddError);
    if(!nmdStatus->AddNetwork(IfName, ssid, key, hexkey))
    {
        qDebug() << "Error adding network " << ssid;
        nmdStatus->startMonitoring();
    }
}

void NetworkTray::slotWifiConnectNetwork(QString ssid, int netid)
{
    connectWifiNetwork(ssid);
}

void NetworkTray::slotWifiEnableError(QString ssid, QString result)
{
    qDebug() << "Error enabling network " << ssid;
    QMessageBox::information(this, QString("ERROR"), QString("Cannot enable network ") + ssid);
    nmdStatus->startMonitoring();
}

void NetworkTray::slotWifiAddError(QString ssid, QString result)
{
    qDebug() << "Error adding network " << ssid;
    QMessageBox::information(this, QString("ERROR"), QString("Cannot add network ") + ssid);
    nmdStatus->startMonitoring();
}

void NetworkTray::updateStatus()
{
    QAction* new_action = nullptr;
    QString status;
    QIcon void_icon;
    bool found = false;
    QString strnet;
    char *ch = nullptr;

    if( (QDateTime::currentDateTime().toSecsSinceEpoch()-LastUpdateTime.toSecsSinceEpoch())*1000 < UPDATE_MSEC/2)
        return;

    if(IfName.startsWith(IF_WIFI_PREFIX))
    {
        // Update connected network menu item and tray icon
        QPair<QString,int> wifi_status = nmdStatus->getLastIfWifiNetwork(IfName);
        //getIfMedia(IfName) if the status is not "Associated" - we should not consider this network as connected to
        if(!wifi_status.first.isEmpty())
        {
            if( (wifi_status.first != LastWifiNetwork) || (wifi_status.second != LastWifiPower) )
            // We need to change tray icon and menu item with current wi-fi network
            {
                status = getWifiStatusLine(wifi_status, true);
                StatusAction->setText(status);
                LastWifiNetwork = wifi_status.first;
                if(wifi_status.second>0)
                {
                    StatusAction->setIcon(getWifiIcon(wifi_status.second));
                    LastWifiPower = wifi_status.second;
                }
                else
                {
                    StatusAction->setIcon(void_icon);
                    LastWifiPower = 0;
                }
                TrayIcon->setIcon(getWifiIcon(wifi_status.second));
            }
        }
        else
        {
            status = ">>> " + tr("disconnected") + " <<<";
            StatusAction->setText(status);
            StatusAction->setIcon(void_icon);
            TrayIcon->setIcon(getWifiIcon(0));
            LastWifiPower = 0;
            LastWifiNetwork = "";
        }
        // Update network list
        QVector<QPair<QString,int>> wifi_networks = nmdStatus->getLastNetsWifiPower(IfName);
        // First outer cycle - by networks
        foreach(const auto &net, wifi_networks)
        {
            //if(wifi_status.first.isEmpty())
            //    continue;
            if(net.first == wifi_status.first)
                continue;
            found = false;
            QMutableVectorIterator<QAction*> actions(ConnectActions);
            // First inner cycle - by actions
            while (actions.hasNext())
            {
                auto action = actions.next();
                if(net.first == action->text().split(" ").at(0))
                {
                    found = true;
                    if(getWifiStatusLine(net, false) != action->text())
                    {
                        action->setText(getWifiStatusLine(net, false));
                        TrayIconMenu->update();
                    }
                    break;
                }
            }
            if( (!found) && (net.first != wifi_status.first) )
            // Action not found for this network, we need to create it, add to ConnectActions list and insert into TrayIconMenu
            {
                new_action = new QAction(getWifiStatusLine(net, false));
                if(net.second>0)
                    new_action->setIcon(getWifiIcon(net.second));
                connect(new_action, &QAction::triggered, this, &NetworkTray::slotConnect);
                TrayIconMenu->insertAction(SeparatorActions[2], new_action);
                ConnectActions.append(new_action);
                TrayIconMenu->update();
            }
        }
        // Second outer cycle - by actions
        for(int i=0; i<ConnectActions.length(); i++)
        {
            QAction* action = ConnectActions[i];
            if(action == nullptr)
                continue;
            if(action->text().isEmpty())
                continue;
            strnet = action->text().split(" ").at(0);
            if(strnet.isEmpty())
                continue;
            found = false;
            // Second inner cycle - by networks
            foreach(const auto &net, wifi_networks)
            {
                if(net.first == wifi_status.first)
                    continue;
                if(net.first == strnet)
                {
                    found = true;
                    break;
                }
            }
            if(!found)
            // Network not found, we need to remove the action from TrayIconMenu and from ConnectActions list
            {
                TrayIconMenu->removeAction(action);
                ConnectActions.removeOne(action);
            }
        }
    }
    else
    // Update wire connection status
    {
    }
    LastUpdateTime = QDateTime::currentDateTime();
}

void NetworkTray::slotShowIfStatus()
{
    QString status = nmdStatus->getLastIfStatusString(IfName);
    QMessageBox::information(this, IfName+" "+tr("Status"), status);
}

void NetworkTray::slotShowError(QString message, QString source)
{
    QMessageBox::critical(this, "Error from "+source, message);
}

QIcon NetworkTray::getWifiIcon(int power)
{
    if(power<25)
        return IconWifiDisconnect;
    else if(power<40)
        return IconWifiConnect25;
    else if(power<70)
        return IconWifiConnect50;
    else if(power<90)
        return IconWifiConnect75;
    else
        return IconWifiConnect;
}

void  NetworkTray::openConfigDlg() {
        if(EthConfig!=nullptr)
            delete EthConfig;   // TODO: What if we open multiple EthConfig dialogs??
        EthConfig = new ethernetconfig();
        EthConfig->setDevice(IfName);
        EthConfig->setWindowTitle(IfName + " " + "configuration");
        EthConfig->setIsMain(false);
        EthConfig->programInit();
        EthConfig->show();
}

void  NetworkTray::openWlanConfigDlg() {
        if(WlanConfig!=nullptr)
            delete WlanConfig;   // TODO: What if we open multiple WlanConfig dialogs??
        WlanConfig = new wificonfigwidgetbase();
        WlanConfig->setIsMain(false);
        WlanConfig->setWindowTitle("WLAN " + IfName + " " + "configuration");
//        WlanConfig->programInit();
        WlanConfig->show();
        WlanConfig->setDevice(IfName);
}

void NetworkTray::resetWpaSupplicant() {
    QString req_string = QString::asprintf(CMD_RESET_WPA.c_str(), IfName.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    nmdStatus->stopMonitoring();
    if(!nmdStatus->sendRequest(req_object))
        QMessageBox::warning(this, IfName, "Cannot send WPA_RESET command to backend");
    nmdStatus->startMonitoring();
}

void NetworkTray::scanWifi()
{
    nmdStatus->stopMonitoring();
    nmdStatus->updateStatus();
    nmdStatus->startMonitoring();
}

void NetworkTray::setIdleScanTimeout()
{
    nmdStatus->setInterval(IDLE_REFRESH_TIME_MSEC);
}

void NetworkTray::setActiveScanTimeout()
{
    nmdStatus->stopMonitoring();
    nmdStatus->updateStatus();
    nmdStatus->setInterval(ACTIVE_REFRESH_TIME_MSEC);
    nmdStatus->startMonitoring();
}

// Load all the icons we'll be using
void NetworkTray::loadIcons() {
#ifdef USE_SVG
    IconWiredConnect.addFile(PROGPATH + "/pics/scalable/nm-adhoc.svg");
    IconWiredDisconnect.addFile( PROGPATH + "/pics/scalable/nm-no-connection.svg");
    IconWifiConnect.addFile( PROGPATH + "/pics/scalable/nm-signal-100.svg");
    IconWifiDisconnect.addFile( PROGPATH + "/pics/scalable/nm-signal-00.svg");
    IconWifiConnect25.addFile( PROGPATH + "/pics/scalable/nm-signal-25.svg");
    IconWifiConnect50.addFile( PROGPATH + "/pics/scalable/nm-signal-50.svg");
    IconWifiConnect75.addFile( PROGPATH + "/pics/scalable/nm-signal-75.svg");
    IconLocked.addFile( PROGPATH + "/pics/object-locked.png");
    IconUnlocked.addFile( PROGPATH + "/pics/object-unlocked.png");
#else
    iconWiredConnect.addFile(PROGPATH + "/pics/24x24/nm-adhoc.png");
    iconWiredDisconnect.addFile( PROGPATH + "/pics/24x24/nm-no-connection.png");
    iconWifiConnect.addFile( PROGPATH + "/pics/24x24/nm-signal-100.png");
    iconWifiDisconnect.addFile( PROGPATH + "/pics/24x24/nm-signal-00.png");
    iconWifiConnect25.addFile( PROGPATH + "/pics/24x24/nm-signal-25.png");
    iconWifiConnect50.addFile( PROGPATH + "/pics/24x24/nm-signal-50.png");
    iconWifiConnect75.addFile( PROGPATH + "/pics/24x24/nm-signal-75.png");
    iconLocked.addFile( PROGPATH + "/pics/object-locked.png");
    iconUnlocked.addFile( PROGPATH + "/pics/object-unlocked.png");
#endif
    IconConfigure = QIcon::fromTheme("cm_options");
    IconQuit = QIcon::fromTheme("application-exit");
    IconReset = QIcon::fromTheme("button_cancel");
    IconScan = QIcon::fromTheme("cm_refresh");
    IconSave = QIcon::fromTheme("filesave");
}

void NetworkTray::slotQuit() {
   exit(0);
}

void NetworkTray::slotGetNetKey(QAction* act){
  if(PICOSESSION){ return; }
  //Get the small SSID from the action
  QString smSSID = act->objectName();
  //trim the small ssid to remove any dots at the end
  smSSID = smSSID.section(".",0,0,QString::SectionSkipEmpty);

  //get the full SSID string
  QString dat = trueos::Utils::runShellCommandSearch("ifconfig -v "+DEVICE->device()+" list scan",smSSID);
  QStringList wdat = NetworkInterface::parseWifiScanLine(dat,true);
  QString SSID = wdat[0];

  //Now forward this SSID connection over to the root-permissioned utility
  qDebug() << "Launch pc-wificonfig to open SSID:" << SSID;
  QProcess::startDetached("sudo pc-wificonfig --connect-ssid \""+SSID+"\" \""+DEVICE->device()+"\"");
  //Get the Security Type
  /*QString sectype = wdat[6];

  if(sectype == "None"){
    //run the Quick-Connect slot without a key
    slotQuickConnect("",SSID, false);

  }else{
    //Open the dialog to prompt for the Network Security Key
    dialogNetKey = new netKey();
    //Insert the SSID into the dialog
    dialogNetKey->setSSID(SSID);
    dialogNetKey->showKeyType( sectype == "WEP" );
    //connect the signal from the dialog to the quick-connect slot
    connect(dialogNetKey,SIGNAL( saved(QString,QString,bool) ),this,SLOT( slotQuickConnect(QString,QString,bool) ) );
    //Activate the dialog
    dialogNetKey->exec();
  }*/
}

/*
void  NetworkTray::displayTooltip() {

  QString  tooltipStr;
  if(PICOSESSION){
    QString sshinfo = getenv("SSH_CONNECTION");
    QString auth = getenv("PICO_CLIENT_LOGIN");
    tooltipStr = QString( tr("PICO Connection: %1") ).arg(auth);
    tooltipStr += "<br>-----------------------------------";
    tooltipStr += "<br>"+ QString(tr("Client IP: %1")).arg(sshinfo.section(" ",0,0));
    tooltipStr += "<br>"+ QString(tr("Host IP: %1")).arg(sshinfo.section(" ",2,2));
  }else{
    //Real device
    tooltipStr = QString(tr("Device Name:") + " /dev/" + DEVICE->device());
    tooltipStr += "<br>" + DEVICE->desc() +"<hr>";
    QString DeviceStatus = DEVICE->mediaStatusAsString();
    QString DeviceMedia = getMediaForIdent();
    if ( !DEVICE->isWireless() )  {
      //WIRED DEVICE
       if ( DeviceStatus == "active" || DeviceStatus == "" ) {
         tooltipStr +=  tr("IP:") + " " + DEVICE->ipAsString();
         tooltipStr += "<br>" + tr("IPv6:") + " " + DEVICE->ipv6AsString();
         tooltipStr += "<br>" + tr("Mac/Ether:") + " " + DEVICE->macAsString();
         tooltipStr += "<br>" + tr("Media:") + " " + DeviceMedia;
         tooltipStr += "<br>" + tr("Status:") + " " + (DEVICE->isUp() ? "UP" : "DOWN");
       } else {
         tooltipStr +=  tr("Mac/Ether:") + " " + DEVICE->macAsString();
         tooltipStr += "<br>" + tr("Media:") + " " + DeviceMedia;
         tooltipStr += "<br>" + tr("Status:") + " " + DeviceStatus;
         tooltipStr += "<br>" + tr("No connection detected.<br> Check your cable connection and try again!");
       }
   } else {
     // If this is a wireless device, give different output
     if ( DeviceStatus == "associated" ) {
      tooltipStr +=  tr("IP:") + " " + DEVICE->ipAsString();
    tooltipStr += "<br>" + tr("IPv6:") + " " + DEVICE->ipv6AsString();
        tooltipStr += "<br>" + tr("SSID:") + " " + DeviceSSID;
        tooltipStr += "<br>" + tr("Connection Strength:") + " " + DeviceSignalStrength + "%";
        tooltipStr += "<br>" + tr("Connection Speed:") + " " + DeviceWirelessSpeed;
    tooltipStr += "<br>" + tr("Mac/Ether:") + " " + DEVICE->macAsString();
    tooltipStr += "<br>" + tr("Media:") + " " + DeviceMedia;
    tooltipStr += "<br>" + tr("Status:") + " " + DeviceStatus;
       } else {
    tooltipStr +=  tr("Mac/Ether:") + " " + DEVICE->macAsString();
    tooltipStr += "<br>" + tr("Media:") + " " + DeviceMedia;
    tooltipStr += "<br>" + tr("Status:") + " " + DeviceStatus;
    tooltipStr += "<br>" + tr("No wireless connection detected.<br> Double-click to start the wireless configuration wizard.");
      }
    }
  } //end check for PICOSESSION

  // Update the tooltop
  TrayIcon->setToolTip(tooltipStr);

}

QString NetworkTray::getLineFromCommandOutput( QString command )
{
  FILE *file = popen(command.toLatin1(),"r");

  char buffer[100];

  QString line = "";
  char firstChar;

  if ((firstChar = fgetc(file)) != -1){
     line += firstChar;
     line += fgets(buffer,100,file);
  }
  pclose(file);
  return line;
}

QString NetworkTray::getSSIDForIdent()
{
  if(PICOSESSION){ return ""; }
  QString inputLine = ifconfigOutput;
  QString SSID = "";

  if (inputLine != "" && inputLine.indexOf("ssid ") != -1){
    SSID = inputLine.remove(0, inputLine.indexOf("ssid ") + 5);
    SSID.truncate(inputLine.indexOf("channel") - 1 );
  }

  return SSID;
}

QString NetworkTray::getSignalStrengthForIdent( QString ident )
{
  if(PICOSESSION){ return ""; }
  // Get the signal strength of this device (User permissions - cannot "up" the device)
  QString command = IFCONFIG + " " + ident + " list scan | grep " + DeviceSSID;
  QString line = getLineFromCommandOutput(command);
  bool ok, ok2;

  QString tmp = line.simplified();

  // Lets find the signal strength / noise variables now
  tmp = tmp.section(" ",4,4);

  // Get the noise
  QString noise = tmp.section(":",1,-1).simplified();
  // Get the signal
  QString sig = tmp.section(":",0,0).simplified();
  //qDebug() << "Signal:" << sig << " Noise:" << noise;

  // Now figure out the percentage
  int isig = sig.toDouble(&ok);
  int inoise = noise.toDouble(&ok2);

  int percent;
  if ( ok && ok2 ) {
    //Special case - FreeBSD bug workaround (signal/noise numbers reversed?)
    if( (isig<inoise) && ( (inoise-isig)>20) ){
      //if the noise is reported as more than 20dB stronger than the signal, FreeBSD has probably got the Sig/Noise values reversed
      //signal values are supposed to hang around the ~-120-80 range, and stronger than the noise
      // The general noise dB is usually -150 to -120
      int tmp = isig;
      isig = inoise;
      inoise = tmp;
    }
     percent = (isig - inoise) * 4; //Note: These are both typically negative values (-74Db signal, -96Db noise for example)
     if(percent<0 && (isig>0 || inoise>0) ){
       //Some weird result - never supposed to be positive values as input
       percent = qAbs(percent); //just flip the sign for now -the difference between signal/noise is still the key value
     }
  //qDebug() << "Sig/Noise Calc:" << tmp << sig << noise << percent;
     // Sanity check
     if ( percent > 100 )
    percent = 100;
     if ( percent < 0 )
    percent = 0;
     tmp.setNum(percent);
  } else {
     tmp = "";
  }
  return tmp;
}

QString NetworkTray::getWirelessSpeedForIdent( QString ident )
{
  if(PICOSESSION){ return ""; }
  QString command = IFCONFIG + " " + ident + " list scan | grep " + DeviceSSID;
  QString line = getLineFromCommandOutput(command);
  QString tmp;

  // Get the signal strength of this device
  tmp = line;
  tmp = tmp.simplified();

  // First remove the SSID which may have spaces in it
  tmp.remove(0, tmp.indexOf(":"));

  // Now find the Speed section
  tmp = tmp.section(" ", 2, 2);
  return tmp;
}

void NetworkTray::slotTrayActivated(QSystemTrayIcon::ActivationReason reason) {
   qDebug() << "slotTrayActivated";
   if(reason == QSystemTrayIcon::Trigger) {
      //openNetManager();
      openConfigDlg();
   }
}

void NetworkTray::monitorStatus(bool noloop) {
  if(PICOSESSION){ updateWifiNetworks(); return; } //nothing to monitor - just create tray menu
  // Start checking to see if the device has changed, and if it has inform the user
  //QString tmp;  
  QIcon Icon;
  QString NotifyText = "";

  // Check the media status of this device
  QString DeviceMedia = getMediaForIdent();

  //Only get general info once for each run
  bool iswifi = DEVICE->isWireless();
  bool isup = DEVICE->isUp();

  // First check the status of the device
  QString DeviceStatus = DEVICE->mediaStatusAsString();
  if ( !iswifi )
  {
    if ( (DeviceStatus == "active" || DeviceStatus == "")  && isup)
      Icon = IconWiredConnect;
    else
      Icon = IconWiredDisconnect;

  } else {
    if ( DeviceStatus == "associated" && isup )
      Icon = IconWifiConnect;
    else
      Icon = IconWifiDisconnect;
  }

  if ( iswifi && DeviceStatus == "associated" )
  {
    // Now check the SSID Address for changes
    DeviceSSID = getSSIDForIdent();

    if (! DeviceSSID.isEmpty() )
    {
       int newStrength = 0;
       int iconStrength = 0;
       QString FileLoad;
       bool ok;
       // Get the strength of the signal
       QString tmp = getSignalStrengthForIdent( DEVICE->device() );
       DeviceSignalStrength = tmp;
       // Figure out if we need to change the strength icon
       tmp.toInt(&ok);
       if ( ok ) {
         newStrength = tmp.toInt(&ok);
	 if ( newStrength < 5 )
         {	
	   iconStrength = 0;
           Icon = IconWifiConnect;
         } else if ( newStrength < 50 ) {
	   iconStrength = 30;
           Icon = IconWifiConnect25;
	 } else if ( newStrength < 75 ) {
	   iconStrength = 60;
           Icon = IconWifiConnect50;
         } else {
	   iconStrength = 85;
           Icon = IconWifiConnect75;
	 }
       } else {
	  DeviceSignalStrength = tr("Unknown");
       }
       // Get the connection speed being used
       DeviceWirelessSpeed = getWirelessSpeedForIdent( DEVICE->device() );
     }

  }

  // Set the tray icon now
  TrayIcon->setIcon(Icon);


//  displayTooltip();
//  updateWifiNetworks(); **************ipfw: socket: Operation not permitted
  
  if(!noloop){
    QTimer::singleShot(UPDATE_MSEC,  this,  SLOT(monitorStatus()));
  }
}
*/

// If the user wants to restart the network, do so
/*
void NetworkTray::slotRestartNetwork() {
  if(PICOSESSION){ return; }
  //trayIcon->showMessage( tr("Please Wait"),tr("Restarting Network"),QSystemTrayIcon::NoIcon,5000);  
  QProcess::startDetached("qsudo service network restart");
}

void NetworkTray::openNetManager() {
  if(PICOSESSION){ return; }
  QString arguments = "pc-netmanager";
  if(getuid()!=0){ arguments.prepend("sudo "); }
  trueos::Utils::runShellCommand(arguments);
}

void  NetworkTray::openDeviceInfo() {
  if(PICOSESSION){ return; }
  QString program = "sudo";
  QStringList arguments;
  if ( !DEVICE->isWireless() )
  {
    arguments << "pc-ethernetconfig" << "info" << DEVICE->device();
  } else {
    arguments << "pc-wificonfig" << "info" << DEVICE->device();
  }

  QProcess *runCommandProc = new QProcess(this);
  runCommandProc->start(program, arguments);
}

QString NetworkTray::getMediaForIdent()
{
  if(PICOSESSION){ return ""; }
  QString inputLine = ifconfigOutput;
  QString status = "";

  if (inputLine != "" && inputLine.indexOf("media:") != -1){
    status = inputLine.remove(0, inputLine.indexOf("media:") + 7);
    status.truncate(inputLine.indexOf("status:") - 1 );
  }
	
  return status;
}

void NetworkTray::slotUpdateIfStatus()
{
  if(PICOSESSION){ return; }
   QProcess *getIfProc = new QProcess();
   getIfProc->start(IFCONFIG, QStringList() << DEVICE->device());
   while(getIfProc->state() == QProcess::Starting || getIfProc->state() == QProcess::Running) {
      getIfProc->waitForFinished(50);
      //QCoreApplication::processEvents();
   }

   ifconfigOutput.clear();
   ifconfigOutput = getIfProc->readAllStandardOutput().simplified();

   //qDebug() << IFCONFIG + " output: " << ifconfigOutput <<"\n";

   getIfProc->kill();
   delete getIfProc;

   QTimer::singleShot(UPDATE_MSEC, this,  SLOT(slotUpdateIfStatus()) );
}

void NetworkTray::slotCheckWifiAvailability(){
  if(PICOSESSION){ return; }
  if( DEVICE->isWireless() ){
    //Show a message if the wifi is down
    if( !DEVICE->isUp() ){
      TrayIcon->showMessage( tr("No Wireless Network Connection"),tr("Click here to configure wireless connections"),QSystemTrayIcon::NoIcon,15000);
    }
  }
}

void NetworkTray::updateWifiNetworks(){
  // Change the right-click of the tray icon to show all available wireless networks
   
 //Redo the tray menu
  if(TrayIconMenu==0){
    TrayIconMenu = new QMenu(this);
    trayActionGroup = new QActionGroup(this);
    //Connect the actionGroup signal with slotQuickConnect
    QObject::connect(trayActionGroup, SIGNAL(triggered(QAction*)),this,SLOT(slotGetNetKey(QAction*)));
  }else{
    TrayIconMenu->clear();
  }
if(!PICOSESSION){
  TrayIconMenu->addSeparator();
  QIcon ssidIcon;
  //update the list of wifi networks available
  QString cmd = "ifconfig "+DEVICE->device()+" list scan";
  QStringList wifinet = trueos::Utils::runShellCommand(cmd);
 //add an entry for each wifi network detected
  for(int i=1; i<wifinet.length(); i++){ //skip the first line (labels)
    //Get the ssid and Security for this network
    QStringList wdata = NetworkInterface::parseWifiScanLine(wifinet[i],false);
    QString ssid = wdata[0];
    QString strength = wdata[4];
    //Add this network to the list
    QString entry = ssid+" ("+strength+")"; // SSID (%Strength)
    //Get the proper "locked" or "unlocked" icon for the network
    if(wdata[6].contains("None")){
      ssidIcon = IconUnlocked;
    }else{
      ssidIcon = IconLocked;
    }
    //Make sure there is an ssid (don't show blank entry points)
    if( !ssid.isEmpty() ){

    //Check for duplicate SSID entries
    bool duplicateSSID = false;
    int dupRow;
    for ( int z = 0 ; z < TrayIconMenu->actions().count() ; z++){
      //qDebug() << listWifi->item(z)->text() << ssid;
      if ( TrayIconMenu->actions()[z]->text().contains(ssid+" (") ){
        duplicateSSID = true;
        dupRow = z;
        break;
      }
    }
    if(!duplicateSSID){
      //Create the action
      QAction* tempAct = new QAction(entry, this); //set the label for the action on creation
      tempAct->setObjectName(wdata[0]); //set the action name as the SSID
      tempAct->setIcon(ssidIcon); //set the action icon
      //add the action to the action group
      trayActionGroup->addAction(tempAct); 
      //Add the action to the menu
      TrayIconMenu->addAction(tempAct);
    }else{
      //Check if the new signal strength is greater, replace if it is
      QString oldStrength = TrayIconMenu->actions()[dupRow]->text();
      oldStrength = oldStrength.section("(",1,1).section("%)",0,0).simplified();
      if( strength.section("%",0,0).toInt() > oldStrength.toInt() ){
        //New signal strength is greater - replace the string
        //qDebug() << "Changing signal strength for "+ssid+" to reflect a stronger signal found";
        //qDebug() << strength << oldStrength;
        TrayIconMenu->actions()[dupRow]->setText( ssid+" ("+strength+")" );
      }
    }
    } //end of the empty ssid check
  } //end loop over wifinet

  //Add the configuration options to the bottom
  TrayIconMenu->addSeparator();
  TrayIconMenu->addAction( tr("Configure ") + DEVICE->device(), this, SLOT(openConfigDlg()));
  TrayIconMenu->addAction( tr("Start the Network Manager"), this, SLOT(openNetManager()));
  TrayIconMenu->addSeparator();
  TrayIconMenu->addAction( tr("Restart the Network"), this, SLOT(slotRestartNetwork()));
  if(QFile::exists("/usr/local/bin/enable-tor-mode")){
    QAction *tmp =  TrayIconMenu->addAction(tr("Route through TOR") );
    tmp->setCheckable(true);
    tmp->setChecked(checkTorMode());
    connect(tmp, SIGNAL(toggled(bool)), this, SLOT(slotToggleTorMode(bool)) );
  }
}// end PICOSESSION check - PICO sessions don't need anything other than "close"
  TrayIconMenu->addAction( tr("Close the Network Monitor"), this, SLOT(slotQuit()));
  
  //attach the new menu to the tray
  TrayIcon->setContextMenu(TrayIconMenu);
 
}

void NetworkTray::slotQuickConnect(QString key,QString SSID, bool hexkey){
  if(PICOSESSION){ return; }
  
  //Run the wifiQuickConnect function
  NetworkInterface::wifiQuickConnect(SSID,key,DEVICE->device(), hexkey);
  
  //Inform the user that it is connecting (this is done by libtrueos now)
  //QString msg = tr("Connecting to ") + SSID;
  //trayIcon->showMessage( tr("Please Wait"),msg,QSystemTrayIcon::NoIcon,10000);
}

void NetworkTray::slotToggleTorMode(bool enable){
  if(PICOSESSION){ return; }
  if(enable){ QProcess::startDetached("qsudo enable-tor-mode"); }
  else{ QProcess::startDetached("qsudo disable-tor-mode"); }
}

bool NetworkTray::checkTorMode(){
  if(PICOSESSION){ return false; }
  static bool lastcheck = false;
  bool running = (0 == QProcess::execute("enable-tor-mode -c") );// 0 if *in* TOR mode, 1 if not
  if(lastcheck != running && running){
    QProcess::startDetached("xdg-open https://check.torproject.org");
  }
  lastcheck = running;
  return running;
}
*/
