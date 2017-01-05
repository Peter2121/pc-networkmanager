
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
#include <iostream>

#include "NetworkTray.h"
#include <trueos-netif.h>
#include <trueos-utils.h>

/* Update interval in ms */
#define  UPDATE_MSEC 15000

#define PREFIX QString("/usr/local")

// Public Variables
QString DeviceSSID;
QString DeviceSignalStrength = "";
int 	DeviceSavedStrength = 0;
QString DeviceWirelessSpeed = "";

QString username;

extern bool PICOSESSION;

void NetworkTray::programInit(QString Device)
{
  //Setup all the null pointers to start with
  runCommandProc = 0;
  getIfProc = 0;
  trayIconMenu = 0;
  trayActionGroup = 0;
  fileWatcherClosed = 0;
  DEVICE = 0;
  // Load the icons we'll be using into memory
  loadIcons();
  trayIcon = new QSystemTrayIcon(this);

  if(!PICOSESSION){
    QString tmp;
    QIcon Icon;
    DEVICE = new backend::NetDevice(Device);

    QString cmd = IFCONFIG + " lagg0 2>/dev/null | grep " + Device;
    QString checkLagg = getLineFromCommandOutput(cmd.toLatin1());
    if ( ! checkLagg.isEmpty() )
      usingLagg = true;
    else
      usingLagg = false;
    // Get the username of the person we are running as
    username = QString::fromLocal8Bit(getenv("LOGNAME"));
  
    // Confirm this is a legit device
    confirmDevice(Device); 

    // Update the ifconfig line we will be parsing
    slotUpdateIfStatus();
  
    // Connect our double-click slot and message clicked slot
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(slotTrayActivated(QSystemTrayIcon::ActivationReason)));
    connect(trayIcon, SIGNAL( messageClicked() ), this,SLOT( openConfigDlg() ));
  }

  trayIcon->setIcon(PICOSESSION ? iconWiredConnect : iconWifiDisconnect);
  trayIcon->show();
  
  //Display a message about the wireless status
  if(!PICOSESSION){
    QTimer::singleShot(5000,this,SLOT(slotCheckWifiAvailability() ));
  }
  // Create the tooltip popup now
  displayTooltip();

  // Start the monitor service on a refresh loop
  monitorStatus();

}


void NetworkTray::confirmDevice( QString device )
{
  if(PICOSESSION){ return; }
   QString command = IFCONFIG + " -l | grep " + device;
   QString line = getLineFromCommandOutput(command);
   if ( line.isEmpty() )
   {
      // Opps, this device doesn't seem to exist!
      exit(1);
   }
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
  QString tmp, sig, noise;
  bool ok, ok2;
  int isig, inoise, percent;
	
  tmp = line.simplified();

  // Lets find the signal strength / noise variables now
  tmp = tmp.section(" ",4,4);

  // Get the noise
  noise = tmp.section(":",1,-1).simplified();
  // Get the signal
  sig = tmp.section(":",0,0).simplified();


  //qDebug() << "Signal:" << sig << " Noise:" << noise;

  // Now figure out the percentage
  isig = sig.toInt(&ok);
  inoise = noise.toInt(&ok2);
  if ( ok && ok2 ) {
     percent = (qAbs(isig) - qAbs(inoise)) * 4;
  qDebug() << "Sig/Noise Calc:" << tmp << sig << noise << percent;
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
   if(reason == QSystemTrayIcon::Trigger) {
      //openNetManager();
      openConfigDlg();
   }
}


void  NetworkTray::openConfigDlg() {
  if(PICOSESSION){ return; }
  if ( !DEVICE->isWireless() )
  {
    QString program = "sudo";
    QStringList arguments;
    arguments << "pc-ethernetconfig" << DEVICE->device();
    QProcess *runCommandProc = new QProcess(this);
    runCommandProc->start(program, arguments);
  } else {
    QString program = "sudo";
    QStringList arguments;
    arguments << "pc-wificonfig" << DEVICE->device();
    QProcess *runCommandProc = new QProcess(this);
    runCommandProc->start(program, arguments);
  }

}

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
  trayIcon->setToolTip(tooltipStr);
   
}

// Load all the icons we'll be using
void NetworkTray::loadIcons() {
  iconWiredConnect.addFile(PROGPATH + "/pics/tray_local.png");
  iconWiredDisconnect.addFile( PROGPATH + "/pics/tray_local_dis.png");
  iconWifiConnect.addFile( PROGPATH + "/pics/tray_wifi.png");
  iconWifiDisconnect.addFile( PROGPATH + "/pics/tray_wifi_dis.png");
  iconWifiConnect30.addFile( PROGPATH + "/pics/tray_wifi30.png");
  iconWifiConnect60.addFile( PROGPATH + "/pics/tray_wifi60.png");
  iconWifiConnect85.addFile( PROGPATH + "/pics/tray_wifi85.png");
  iconLocked.addFile( PROGPATH + "/pics/object-locked.png");
  iconUnlocked.addFile( PROGPATH + "/pics/object-unlocked.png");
}

void NetworkTray::slotQuit() {
   exit(0);
}

void NetworkTray::slotTriggerFileChanged() {
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
      Icon = iconWiredConnect;
    else
      Icon = iconWiredDisconnect;

  } else {
    if ( DeviceStatus == "associated" && isup )
      Icon = iconWifiConnect;
    else
      Icon = iconWifiDisconnect;
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
           Icon = iconWifiConnect;
         } else if ( newStrength < 50 ) {
	   iconStrength = 30;
           Icon = iconWifiConnect30;
	 } else if ( newStrength < 75 ) {
	   iconStrength = 60;
           Icon = iconWifiConnect60;
         } else {
	   iconStrength = 85;
           Icon = iconWifiConnect85;
	 }
       } else {
	  DeviceSignalStrength = tr("Unknown");
       }
       // Get the connection speed being used
       DeviceWirelessSpeed = getWirelessSpeedForIdent( DEVICE->device() );
     }

  }

  // Set the tray icon now
  trayIcon->setIcon(Icon);


  displayTooltip();
  updateWifiNetworks();
  
  if(!noloop){
    QTimer::singleShot(UPDATE_MSEC,  this,  SLOT(monitorStatus()));
  }

}

// If the user wants to restart the network, do so
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
      trayIcon->showMessage( tr("No Wireless Network Connection"),tr("Click here to configure wireless connections"),QSystemTrayIcon::NoIcon,15000);
    }
  }
}

void NetworkTray::updateWifiNetworks(){
  // Change the right-click of the tray icon to show all available wireless networks
   
 //Redo the tray menu
  if(trayIconMenu==0){
    trayIconMenu = new QMenu(this);
    trayActionGroup = new QActionGroup(this);
    //Connect the actionGroup signal with slotQuickConnect
    QObject::connect(trayActionGroup, SIGNAL(triggered(QAction*)),this,SLOT(slotGetNetKey(QAction*)));
  }else{
    trayIconMenu->clear();
  }
if(!PICOSESSION){
  trayIconMenu->addSeparator();
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
      ssidIcon = iconUnlocked;
    }else{
      ssidIcon = iconLocked;
    }
    //Make sure there is an ssid (don't show blank entry points)
    if( !ssid.isEmpty() ){

    //Check for duplicate SSID entries
    bool duplicateSSID = false;
    int dupRow;
    for ( int z = 0 ; z < trayIconMenu->actions().count() ; z++){
      //qDebug() << listWifi->item(z)->text() << ssid;
      if ( trayIconMenu->actions()[z]->text().contains(ssid+" (") ){
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
      trayIconMenu->addAction(tempAct);
    }else{
      //Check if the new signal strength is greater, replace if it is
      QString oldStrength = trayIconMenu->actions()[dupRow]->text();
      oldStrength = oldStrength.section("(",1,1).section("%)",0,0).simplified();
      if( strength.section("%",0,0).toInt() > oldStrength.toInt() ){
        //New signal strength is greater - replace the string
        //qDebug() << "Changing signal strength for "+ssid+" to reflect a stronger signal found";
        //qDebug() << strength << oldStrength;
        trayIconMenu->actions()[dupRow]->setText( ssid+" ("+strength+")" );
      }
    }
    } //end of the empty ssid check
  } //end loop over wifinet

  //Add the configuration options to the bottom
  trayIconMenu->addSeparator();
  trayIconMenu->addAction( tr("Configure ") + DEVICE->device(), this, SLOT(openConfigDlg()));
  trayIconMenu->addAction( tr("Start the Network Manager"), this, SLOT(openNetManager()));
  trayIconMenu->addSeparator();
  trayIconMenu->addAction( tr("Restart the Network"), this, SLOT(slotRestartNetwork()));
  if(QFile::exists("/usr/local/bin/enable-tor-mode")){
    QAction *tmp =  trayIconMenu->addAction(tr("Route through TOR") );
    tmp->setCheckable(true);
    tmp->setChecked(checkTorMode());
    connect(tmp, SIGNAL(toggled(bool)), this, SLOT(slotToggleTorMode(bool)) );
  }
}// end PICOSESSION check - PICO sessions don't need anything other than "close"
  trayIconMenu->addAction( tr("Close the Network Monitor"), this, SLOT(slotQuit()));
  
  //attach the new menu to the tray
  trayIcon->setContextMenu(trayIconMenu); 
 
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
