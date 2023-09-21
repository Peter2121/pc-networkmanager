/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QDebug>
#include <QInputDialog>
#include <QXmlStreamReader>

#include <trueos-utils.h>
#include <trueos-netif.h>
#include <unistd.h>

#include "wificonfigwidgetbase.h"
#include "ui_wificonfigwidgetbase.h"

QString saveSSID; //need this to pass to a couple slots rather than requiring a currently available wifi

wificonfigwidgetbase::wificonfigwidgetbase() : QWidget()
{
    setupUi(this);
    WPAONLY=false;
    nmd_status = new NMDaemonCheckStatus();
    OwnNmdStatus = true;
}

wificonfigwidgetbase::wificonfigwidgetbase(NMDaemonCheckStatus* nmd) : QWidget()
{
    setupUi(this);
    WPAONLY=false;
    nmd_status = nmd;
    OwnNmdStatus = false;
}

void wificonfigwidgetbase::slotClose()
{
    if(IsMain)
      exit(0);
    else
      this->close();
}

void wificonfigwidgetbase::autoConnectSSID(QString ssid){
  //Public function for launching a connection to a designated SSID *instantly* on UI open
  // - Do any pre-connection setup

  // - Launch the SSID password dialog
  connect(this, SIGNAL(connectSSID(QString)), this, SLOT(slotAddNewProfileSSID(QString)) );
  emit connectSSID(ssid);
}

void wificonfigwidgetbase::slotProfileSaved()
{
    pushApply->setEnabled(true);
}

void wificonfigwidgetbase::slotApply()
{
    QString tmp;
    QString ifConfigLine;
    QVector<int> IndexToModify;
    QVector<int> IdToRemove;
    if ( ! pushApply->isEnabled() )
        return;

    // See if we need to enable a country code
/*
    if ( groupCountryCode->isChecked() )
    {
       tmp = comboCountryCode->currentText().section("(", 1, 1);
       tmp = tmp.section(")", 0, 0);
       ifConfigLine+=" country " + tmp;
    }
*/

/*
    // Check if we need to enable LAGG
    if ( usingLagg )
      NetworkInterface::enableLagg(DeviceName);
*/
    // Apply priority, SSID, BSSID and enable/disable
    int imax = WPANetworksSystem.length();
    foreach (auto net, WPANetworksDialog) {
        for(int i=0; i<imax; i++)
        {
            if(WPANetworksSystem[i].GetId() == net.GetId())
            {
                if( (WPANetworksSystem[i].GetPriority() != net.GetPriority()) ||
                     (WPANetworksSystem[i].IsEnabled() != net.IsEnabled()) ||
                     (WPANetworksSystem[i].GetSsid() != net.GetSsid()) ||
                     (WPANetworksSystem[i].GetBssid() != net.GetBssid()) )
                {
                    if(WPANetworksSystem[i].GetSsid() != net.GetSsid())
                    {
                        auto list_item = listWifi->item(listWifi->currentRow());
                        QString new_text = net.GetSsid();
                        QString new_tip = "SSID: " + net.GetSsid() + "\nPriority: " + QString::number(net.GetPriority());
                        if(!net.IsEnabled())
                        {
                            new_text += " [disabled]";
                            new_tip += "\n[disabled]";
                        }
                        if(net.IsCurrent())
                        {
                            new_text += " [current]";
                            new_tip += "\n[current]";
                        }
                        list_item->setText(new_text);
                        list_item->setToolTip(new_tip);
                    }
                    WPANetworksSystem[i].SetPriority(net.GetPriority());
                    WPANetworksSystem[i].SetSsid(net.GetSsid());
                    WPANetworksSystem[i].SetBssid(net.GetBssid());
                    net.IsEnabled() ? WPANetworksSystem[i].Enable() : WPANetworksSystem[i].Disable();
                    IndexToModify.push_back(i);                    
                }
            }
        }
    }
    foreach (int i, IndexToModify) {
        int netid = WPANetworksSystem[i].GetId();
        nmd_status -> SetNetworkEnabled(DeviceName, netid, WPANetworksSystem[i].IsEnabled());
        nmd_status -> SetNetworkPriority(DeviceName, netid, WPANetworksSystem[i].GetPriority());
        nmd_status -> SetNetworkSsid(DeviceName, netid, WPANetworksSystem[i].GetSsid());
        nmd_status -> SetNetworkBssid(DeviceName, netid, WPANetworksSystem[i].GetBssid());
        nmd_status -> saveConfig(DeviceName);
    }

    // Apply remove
    imax = WPANetworksDialog.length();
    foreach (auto net, WPANetworksSystem) {
        bool found = false;
        for(int i=0; i<imax; i++)
        {
            if(WPANetworksDialog[i].GetId() == net.GetId())
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            IdToRemove.push_back(net.GetId());
        }
    }
    foreach (int id, IdToRemove) {
        nmd_status -> RemoveNetwork(DeviceName, id);
        nmd_status -> saveConfig(DeviceName);
    }

    buttonCancel->setEnabled(true);
    pushApply->setEnabled(false);
}

/*
void wificonfigwidgetbase::updateWPASupp()
{
    bool listEmpty = false;
    QString tmp;

    QFile fileout( "/etc/wpa_supplicant.conf" );
    if ( fileout.open( QIODevice::WriteOnly ) ) {
       QTextStream streamout( &fileout );


       // Loop through all the devices we have now
       for(int curItem=0; curItem < 145; curItem++)
       {
         if ( SSIDList[curItem].isEmpty() ){
	   if(curItem==0){ listEmpty=true; }
	   break;
	 }

         // Fix to prevent kernel panic on net restart
	 if ( curItem == 0 )
           streamout << "ctrl_interface=/var/run/wpa_supplicant\n\n";

	 if ( WPAONLY ) {
           streamout << "ap_scan=0\n";
           streamout << "fast_reauth=0\n";
           streamout << "network={\n ssid=\"" + SSIDList[curItem] + "\"\n";
         } else {
           if ( BSSID[curItem] )
             streamout << "network={\n bssid=" + SSIDList[curItem] + "\n";
           else
             streamout << "network={\n ssid=\"" + SSIDList[curItem] + "\"\n";
	 }

         streamout << " priority=" << 145 - curItem << "\n";

	 if ( ! WPAONLY )
           streamout << " scan_ssid=1\n";

         if ( SSIDEncType[curItem] == NO_ENCRYPTION )
         {
            streamout << " key_mgmt=NONE\n";
         } else if (SSIDEncType[curItem] == WEP_ENCRYPTION ) {
           // Ensure we don't end up with invalid key index
	   if ( WEPIndex[curItem] !=0 && WEPIndex[curItem] !=1 && WEPIndex[curItem] !=2 && WEPIndex[curItem] !=3 )
	      WEPIndex[curItem] = 0;

           streamout << " key_mgmt=NONE\n";
           streamout << " wep_tx_keyidx=" + tmp.setNum(WEPIndex[curItem]) + "\n";
           // Check if we are using a plaintext WEP or not
           if ( WEPHex[curItem] )
             streamout << " wep_key" + tmp.setNum(WEPIndex[curItem]) + "=" + WEPKey[curItem] + "\n";
           else
             streamout << " wep_key" + tmp.setNum(WEPIndex[curItem]) + "=\"" + WEPKey[curItem] + "\"\n";

         } else if (SSIDEncType[curItem] == WPA_ENCRYPTION ) {

            streamout << " psk=\"" + WPAPersonalKey[curItem] + "\"\n";

         } else if (SSIDEncType[curItem] == WPAE_ENCRYPTION ) {
	    QString keyType;
	    if ( WPAEKeyMgmt[curItem] == KEYWPAEAP )
	       keyType="WPA-EAP";
	    if ( WPAEKeyMgmt[curItem] == KEY8021X )
	       keyType="IEEE8021X";

            if ( WPAEType[curItem] == 1) {
              streamout << " proto=RSN\n key_mgmt=" << keyType << "\n eap=TLS\n";
	      streamout << " identity=\"" + WPAEIdent[curItem] + "\"\n";
	      if ( ! WPAECACert[curItem].isEmpty() )
	        streamout << " ca_cert=\"" + WPAECACert[curItem] + "\"\n";
	      if ( ! WPAEClientCert[curItem].isEmpty() )
	        streamout << " client_cert=\"" + WPAEClientCert[curItem] + "\"\n";
	      streamout << " private_key=\"" + WPAEPrivKeyFile[curItem] + "\"\n";
	      streamout << " private_key_password=\"" + WPAEPassword[curItem] + "\"\n";

	    } else if (WPAEType[curItem] == 2) {
              streamout << " proto=RSN\n key_mgmt=" << keyType << "\n eap=TTLS\n";
	      streamout << " identity=\"" + WPAEIdent[curItem] + "\"\n";
	      if ( ! WPAEAnonIdent[curItem].isEmpty() )
  	          streamout << " anonymous_identity=\"" + WPAEAnonIdent[curItem] + "\"\n";
	      streamout << " password=\"" + WPAEPassword[curItem] + "\"\n";
	      if ( ! WPAECACert[curItem].isEmpty() )
	        streamout << " ca_cert=\"" + WPAECACert[curItem] + "\"\n";
          switch(WPAEPhase2[curItem]) {
            case 1:
              streamout << " phase2=\"auth=MSCHAPV2\"\n";
              break;
            case 2:
              streamout << " phase2=\"auth=GTC\"\n";
              break;
            case 3:
              streamout << " phase2=\"auth=OTP\"\n";
              break;
            case 4:
              streamout << " phase2=\"auth=PAP\"\n";
              break;
            case 5:
              streamout << " phase2=\"auth=CHAP\"\n";
              break;
            case 6:
              streamout << " phase2=\"auth=MSCHAP\"\n";
              break;
            default: // comboPhase2 Itemindex 0
              streamout << " phase2=\"auth=MD5\"\n";
          }

	    } else if (WPAEType[curItem] == 3) {
              streamout << " proto=RSN\n key_mgmt=" << keyType << "\n eap=PEAP\n";
	      streamout << " identity=\"" + WPAEIdent[curItem] + "\"\n";
	      if ( ! WPAEAnonIdent[curItem].isEmpty() )
  	          streamout << " anonymous_identity=\"" + WPAEAnonIdent[curItem] + "\"\n";
	      streamout << " password=\"" + WPAEPassword[curItem] + "\"\n";
	      if ( ! WPAECACert[curItem].isEmpty() )
	        streamout << " ca_cert=\"" + WPAECACert[curItem] + "\"\n";
	      streamout << " phase1=\"peaplabel=0\"\n";
          switch(WPAEPhase2[curItem]) {
            case 0:
              streamout << " phase2=\"auth=MD5\"\n";
              break;
            case 2:
              streamout << " phase2=\"auth=GTC\"\n";
              break;
            case 3:
              streamout << " phase2=\"auth=OTP\"\n";
              break;
            default: // comboPhase2 Itemindex 1
              streamout << " phase2=\"auth=MSCHAPV2\"\n";
          }

            }
         }

         streamout << "}\n\n";
       } // End of for loop

       fileout.close();
    }

    // If no networks, clear out WPA supplicant.conf and reset rc.conf
    if ( listEmpty  && ! WPAONLY ){
       fileout.remove();
       trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_lagg0", "", -1);
       trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName, "ifconfig_" + DeviceName + "=\"DHCP\"");
    }

}
*/

void wificonfigwidgetbase::setIsMain(bool is_main)
{
    IsMain = is_main;
}

// Function which is called at the program init
void wificonfigwidgetbase::setDevice( QString Device )
{

  // Connect our add / remove buttons for wifi
  connect( pushDown, SIGNAL( clicked() ), this, SLOT( slotMoveDown() ) );
  connect( pushUp, SIGNAL( clicked() ), this, SLOT( slotMoveUp() ) );
  connect( pushEnable, SIGNAL( clicked() ), this, SLOT( slotEnableProfile() ) );
  connect( pushDisable, SIGNAL( clicked() ), this, SLOT( slotDisableProfile() ) );
  connect( pushAddWifi, SIGNAL( clicked() ), this, SLOT( slotAddNewProfile() ) );
  connect( pushEditWifi, SIGNAL( clicked() ), this, SLOT( slotEditProfile() ) );
  connect( pushRemoveWifi, SIGNAL( clicked() ), this, SLOT( slotRemoveProfile() ) );
  connect( pushRescan, SIGNAL( clicked() ), this, SLOT( slotRescan() ) );
  connect( pushAddHidden, SIGNAL( clicked() ), this, SLOT( slotAddHiddenProfile() ) );
  connect( groupCountryCode, SIGNAL( clicked() ), this, SLOT( slotCheckGlobalText() ) );

  // Save the device name for later
  DeviceName = Device;

  // Center the dialog window
  //QDesktopWidget *d = QApplication::desktop();
  //move(d->width() / 3, d->height() / 4);

  // Connect the slot to refresh
  QTimer::singleShot(100,  this,  SLOT(slotFinishLoading() ) );

  // Check if we are using the lagg0 device
  //QSettings settings("PCBSD");
  //usingLagg = settings.value("/pc-netmanager/useLagg", false).toBool();

  //Load the available wifi access points
  slotRescan();

}

void wificonfigwidgetbase::loadCountryCodes()
{
  QFile file("/etc/regdomain.xml");
  if (!file.open(QFile::ReadOnly | QFile::Text))
  return;

  QXmlStreamReader reader;
  QString code, name;
  int comboNum = 0;

  reader.setDevice(&file);
  reader.readNext();
  while (!reader.atEnd()) {
      if (reader.isStartElement()) {
          if (reader.name() == "country") {
	     name="";
	     code = reader.attributes().value("id").toString();
             while (!reader.atEnd()) {
                 if (reader.isEndElement() && reader.name() == "country")
		    break;
                 if (reader.isStartElement() && reader.name() == "name") {
	   	    name = reader.readElementText();
	         }
                 reader.readNext();
             }
	     if ( !name.isEmpty() && !code.isEmpty() && code != "DEBUG" )
		comboCountryCode->addItem(name + " (" + code.toLower() + ")" );
   	     if ( Country == code.toLower() )
	        comboCountryCode->setCurrentIndex(comboNum);

             comboNum++;
	  }
      }
      reader.readNext();
  }

  // Now if we change index, enable Apply Button
  connect( comboCountryCode, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotCheckGlobalText() ) );
}

void wificonfigwidgetbase::slotRescan()
{
    QString strength, ssid, security, FileLoad;
    QStringList ifconfout, ifline;
    int foundItem = 0;

    // Clear the list box and disable the add button
    listNewWifi->clear();
    pushAddWifi->setEnabled(false);
    QVector<WifiNetworkDetails> wifi_networks = nmd_status->getLastNetsWifiDetails(DeviceName);

    // Start the scan and get the output
    //ifconfout = trueos::Utils::runShellCommand("ifconfig -v " + DeviceName + " up list scan");

    foreach(const auto &net, wifi_networks)
    {
        ssid = net.SSID;
//        strength = net.Power;
//      TODO: manage WIFI_SECURITY_ERROR case (don't load any icon)
        if(net.Security == NMDaemonClient::WIFI_SECURITY_OPEN)
            FileLoad = ":object-unlocked.png";
        else
            FileLoad = ":object-locked.png";
        listNewWifi->addItem(new QListWidgetItem(QIcon(FileLoad), ssid + " (signal: " + QString(std::to_string(net.Power).c_str()) + "%)") );
        foundItem = 1;
    }
/*
    //display the info for each wifi access point
    for(int i=1; i<ifconfout.size(); i++){    //Skip the header line by starting at 1
      ifline = NetworkInterface::parseWifiScanLine(ifconfout[i],true); //get a single line
      //save the info for this wifi
      //qDebug() << "ifconfig run";
      ssid = ifline[0];
      strength = ifline[4];
      //determine the icon based on if there is security encryption
      security = ifline[6]; //NetworkInterface::getWifiSecurity(ssid,DeviceName);
      if(security.contains("None")){
	FileLoad = ":object-unlocked.png";
      }else{
	FileLoad = ":object-locked.png";
      }
      //qDebug() << "ssid check";
      //Make sure we do not display blank ssid entry points
      if( !ssid.isEmpty() ){

      // qDebug()<<"Checking for duplicate SSID's";
      bool duplicateSSID = false;
      int dupRow;
      for ( int z = 0 ; z < listNewWifi->count() ; z++){
        //qDebug() << listNewWifi->item(z)->text() << ssid;
	if ( listNewWifi->item(z)->text().contains(ssid+" (") ){
          dupRow = z;
          duplicateSSID = true;
          break;
	}
      }
      if( !duplicateSSID ){
        //Add the wifi access point to the list
        listNewWifi->addItem(new QListWidgetItem(QIcon(FileLoad), ssid + " (signal: " +strength + ")") );
        foundItem = 1; //set the flag for wifi signals found
      }else{
        //Check if the new signal strength is greater, replace if it is
        QString oldStrength = listNewWifi->item(dupRow)->text();
        oldStrength = oldStrength.section("(signal:",1,1).section("%)",0,0).simplified();
        if( strength.section("%",0,0).toInt() > oldStrength.toInt() ){
          //New signal strength is greater - replace the string
          qDebug() << "Changing signal strength for "+ssid+" to reflect a stronger signal found";
          //qDebug() << strength << oldStrength;
          listNewWifi->item(dupRow)->setText( ssid+" (signal: "+strength+")" );
	}
      }
      } //End of check for empty SSID entry points
    }
*/

    if ( foundItem == 1 ){
      listNewWifi->setCurrentRow(-1);
      pushAddWifi->setEnabled(true);
    } else {
      pushAddWifi->setEnabled(false);
    }
}

void wificonfigwidgetbase::refreshNetworksList()
{
    listWifi->clear();
    foreach (const auto &net, WPANetworksDialog)
    {
        QString ssid = net.GetSsid();
        QString net_line = ssid;
        QString net_tip = "SSID: " + ssid + "\nPriority: " + QString::number(net.GetPriority());
        QListWidgetItem* item_net_line = nullptr;
        if(!net.IsEnabled())
        {
            net_line += " [disabled]";
            net_tip += "\n[disabled]";
        }
        if(net.IsCurrent())
        {
            net_line += " [current]";
            net_tip += "\n[current]";
        }
        if(net.IsProtected())
        {
            item_net_line = new QListWidgetItem(QIcon(":object-locked.png"), net_line);
            item_net_line->setToolTip(net_tip);
            listWifi->addItem(item_net_line);
        }
        else
        {
            item_net_line = new QListWidgetItem(QIcon(":object-unlocked.png"), net_line);
            item_net_line->setToolTip(net_tip);
            listWifi->addItem(item_net_line);
        }
    }
    listWifi->setMouseTracking(true);
}


// Slot which moves a SSID higher in the connection priority
void wificonfigwidgetbase::slotMoveUp()
{
    int itomove = -1;

    if ( listWifi->currentRow() != -1 )
    {
        QString move_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == move_ssid )
            {
                itomove = i;
                break;
            }
        }
        if(itomove > 0)
        {
            if(WPANetworksDialog[itomove-1].GetPriority()==WPANetworksDialog[itomove].GetPriority())
                WPANetworksDialog[itomove].IncreasePriority();
            else if(WPANetworksDialog[itomove-1].GetPriority()>WPANetworksDialog[itomove].GetPriority())
                WPANetworksDialog[itomove].SetPriority(WPANetworksDialog[itomove-1].GetPriority()+1);
            std::sort(WPANetworksDialog.begin(), WPANetworksDialog.end());
            std::reverse(WPANetworksDialog.begin(), WPANetworksDialog.end());
            refreshNetworksList();
            pushApply->setEnabled(true);
        }
    }
}

void wificonfigwidgetbase::slotMoveDown()
{
    int itomove = -1;

    if ( listWifi->currentRow() != -1 )
    {
        QString move_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == move_ssid )
            {
                itomove = i;
                break;
            }
        }
        if( (itomove >= 0) && (itomove < WPANetworksDialog.count()-1))
        {
            if(WPANetworksDialog[itomove+1].GetPriority()==WPANetworksDialog[itomove].GetPriority())
                WPANetworksDialog[itomove].DecreasePriority();
            else if(WPANetworksDialog[itomove+1].GetPriority()<WPANetworksDialog[itomove].GetPriority())
                WPANetworksDialog[itomove].SetPriority(WPANetworksDialog[itomove+1].GetPriority()-1);
            std::sort(WPANetworksDialog.begin(), WPANetworksDialog.end());
            std::reverse(WPANetworksDialog.begin(), WPANetworksDialog.end());
            refreshNetworksList();
            pushApply->setEnabled(true);
        }
    }
}

void wificonfigwidgetbase::slotEditProfile()
{
    //bool foundSSID=false;
    //int curItem=0;
    int itoedit = -1;
    std::unique_ptr<WifiEdit> wifi_edit_dialog = nullptr;

    // Check if we have a selection to work with
    if ( listWifi->currentRow() != -1 )
    {
        QString edit_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == edit_ssid )
            {
                itoedit = i;
                break;
            }
        }
        if( (itoedit >= 0) && (itoedit < WPANetworksDialog.count()-1))
        {
            wifi_edit_dialog = std::make_unique<WifiEdit>(&WPANetworksDialog[itoedit]);
            connect( wifi_edit_dialog.get(), SIGNAL( Saved() ), this, SLOT( slotProfileSaved() ) );
            wifi_edit_dialog->setModal(true);
            wifi_edit_dialog->exec();
        }

/*
      // Lets go ahead and open the edit dialog on this SSID
      if (foundSSID)
      {
          // Set our internal flag that this is an edit on an existing device
          wifiselectiondialog wifiselect;
          wifiselect.init(DeviceName);
          wifiselect.setWPAOnly(WPAONLY);

          // Check the type of SSID this is, and issue appropriate edit
          if ( SSIDEncType[curItem] == NO_ENCRYPTION) {
             wifiselect.initEdit(SSIDList[curItem], BSSID[curItem]);
          }
          if ( SSIDEncType[curItem] == WEP_ENCRYPTION) {
             wifiselect.initEdit(SSIDList[curItem], BSSID[curItem], WEPKey[curItem], WEPIndex[curItem], WEPHex[curItem]);
          }
          if ( SSIDEncType[curItem] == WPA_ENCRYPTION) {
             wifiselect.initEdit(SSIDList[curItem], BSSID[curItem], WPAPersonalKey[curItem]);
          }
          if ( SSIDEncType[curItem] == WPAE_ENCRYPTION) {
             wifiselect.initEdit(SSIDList[curItem], BSSID[curItem], WPAEType[curItem], WPAEIdent[curItem], WPAEAnonIdent[curItem], WPAECACert[curItem], WPAEClientCert[curItem], WPAEPrivKeyFile[curItem], WPAEPassword[curItem], WPAEKeyMgmt[curItem], WPAEPhase2[curItem]);
          }


          // Connect our delete signal, which runs before we add a new SSID
          connect( &wifiselect, SIGNAL( signalDeleteSSID(QString) ), this, SLOT( slotRemoveProfileSSID(QString) ) );

          // Connect our save signals
          connect( &wifiselect, SIGNAL( signalSavedOpen(QString, bool) ), this, SLOT( slotAddNewProfileOpen(QString, bool) ) );
          connect( &wifiselect, SIGNAL( signalSavedWEP( QString, bool, QString, int, bool ) ), this, SLOT( slotAddNewProfileWEP( QString, bool, QString, int, bool) ) );
          connect( &wifiselect, SIGNAL( signalSavedWPA(QString, bool, QString) ), this, SLOT( slotAddNewProfileWPA(QString, bool, QString) ) );
          connect( &wifiselect, SIGNAL( signalSavedWPAE(QString, bool, int, QString, QString, QString, QString, QString, QString, int, int) ), this, SLOT ( slotAddNewProfileWPAE(QString, bool, int, QString, QString, QString, QString, QString, QString, int, int) ) );

          wifiselect.exec();
      }
*/

    }
}

void wificonfigwidgetbase::slotRemoveProfile()
{
    if ( listWifi->currentRow() != -1 )
    {
        QString remove_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        slotRemoveProfileSSID(remove_ssid);
        refreshNetworksList();
        pushApply->setEnabled(true);
    }
}

void wificonfigwidgetbase::slotAddHiddenProfile(){
  //Prompt for the desired SSID
  bool ok;
  QString ssidc = QInputDialog::getText(this,tr("Network Name"),tr("Please enter the name of the network you wish to add"), QLineEdit::Normal,"",&ok);
  //check if a name was given
  if( ok && !ssidc.isEmpty()){
    slotAddNewProfileSSID(ssidc.simplified());
  }
}

void wificonfigwidgetbase::slotAddNewProfile()
{
   //Get the selected SSID
   if( (listNewWifi->currentRow())!=-1 ){
    QString line = listNewWifi->item(listNewWifi->currentRow())->text();
    QString ssidc = line.section(" (",0,0,QString::SectionSkipEmpty);
    slotAddNewProfileSSID(ssidc);
   }else{
     QMessageBox::warning(this,"No Wifi Point Selected","Please select a wireless network to add");
   }
}

void wificonfigwidgetbase::slotAddNewProfileSSID(QString ssidc){
  /*  ssidc - SSID of the network to add
  */

    //Get the Security Type
    QString sec = NetworkInterface::getWifiSecurity(ssidc,DeviceName);

    //Save the SSID for the future save slots
    saveSSID = ssidc;

    //Open the proper security dialog and link it to the save slots
    if(sec.contains("None")){
      slotAddNewProfileOpen(ssidc,false); //call the function to save the variables (no security settings)
    }else if(sec.contains("WEP")){
     wepConfig dialogWEP;
     connect( &dialogWEP, SIGNAL( saved(QString, int, bool) ), this, SLOT( slotWEPSave(QString, int, bool) ) );
     dialogWEP.exec();
    }else if(sec.contains("WPA")){ //both WPA and WPA2 encryption
     dialogWPAPersonal dialogWPA;
     connect( &dialogWPA, SIGNAL( saved(QString) ), this, SLOT( slotWPAPSave(QString) ) );
     dialogWPA.exec();
    }else{
      //Put error handling here
      slotAddNewProfileOpen(ssidc,false); //call the function to save the variables (no security settings)
      if ( ! WPAONLY )
        QMessageBox::warning(this,"Unknown Wifi Security","The security for this wifi access point could not be automatically determined.\n Please edit the profile to manually set the proper security encryption.");
    }
}

void wificonfigwidgetbase::slotWEPSave(QString newkey, int newindex, bool hexkey)
{
  //get the ssid
  //QString line = listNewWifi->item(listNewWifi->currentRow())->text();
  //QString ssidc = line.section(" (",0,0,QString::SectionSkipEmpty);
  QString ssidc = saveSSID;
  saveSSID.clear();
  //Add the profile
  slotAddNewProfileWEP(ssidc,false,newkey,newindex,hexkey);
}

void wificonfigwidgetbase::slotWPAPSave(QString newkey)
{
  //get the ssid
  //QString line = listNewWifi->item(listNewWifi->currentRow())->text();
  //QString ssidc = line.section(" (",0,0,QString::SectionSkipEmpty);
  QString ssidc = saveSSID;
  saveSSID.clear();
  //Add the profile
  slotAddNewProfileWPA(ssidc,false,newkey);
}


void wificonfigwidgetbase::runCommand( QString Command )
{
    FILE *file = popen(Command.toLatin1(),"r");
	pclose(file);
}

void wificonfigwidgetbase::slotRemoveProfileSSID(QString remove_ssid)
{
    for (int i=0; i<WPANetworksDialog.count(); i++)
    {
        if ( WPANetworksDialog[i].GetSsid() == remove_ssid )
        {
            WPANetworksDialog.removeAt(i);
            break;
        }
    }
}

void wificonfigwidgetbase::slotAddNewProfileOpen(QString SSID, bool isBSSID)
{
   for (int dupItem = 0; dupItem < 145; dupItem++)
   {
      if ( SSID == SSIDList[dupItem] ) {
          QMessageBox::warning( this, tr("Error"), tr("You already have a wireless network with this SSID! Please remove it first.\n") );
          return;
      }
   }


   int curItem=0;

   while( ! SSIDList[curItem].isEmpty() && curItem < 145 )
   {
     curItem++;
   }

    // Find which current item we are working on

    SSIDList[curItem] = SSID;
    BSSID[curItem] = isBSSID;

    // Refresh the SSID listbox and enable the apply button
    slotRefreshSSIDList();
    slotCheckGlobalText();
}

void wificonfigwidgetbase::slotAddNewProfileWEP( QString SSID, bool isBSSID, QString newKey, int newIndex, bool hexkey )
{
   for (int dupItem = 0; dupItem < 145; dupItem++)
   {
      if ( SSID == SSIDList[dupItem] ) {
        QMessageBox::warning( this, tr("Error"), tr("You already have a wireless network with this SSID! Please remove it first.\n") );
        return;
      }
   }


   int curItem=0;

   while( ! SSIDList[curItem].isEmpty() && curItem < 145 )
   {
     curItem++;
   }

    // Find which current item we are working on

    SSIDList[curItem] = SSID;
    SSIDEncType[curItem] = WEP_ENCRYPTION;
    BSSID[curItem] = isBSSID;
    WEPKey[curItem] = newKey;
    WEPIndex[curItem] = newIndex;
    WEPIndex[curItem]--;
    WEPHex[curItem] = hexkey;

    // Refresh the SSID listbox and enable the apply button
    slotRefreshSSIDList();
    slotCheckGlobalText();

}


void wificonfigwidgetbase::slotAddNewProfileWPA( QString SSID, bool isBSSID, QString newKey )
{
   for (int dupItem = 0; dupItem < 145; dupItem++)
   {
      if ( SSID == SSIDList[dupItem] ) {
        QMessageBox::warning( this, tr("Error"), tr("You already have a wireless network with this SSID! Please remove it first.\n") );
        return;
      }
   }

   int curItem=0;

   while( ! SSIDList[curItem].isEmpty() && curItem < 145 )
   {
     curItem++;
   }


   SSIDList[curItem] = SSID;
   SSIDEncType[curItem] = WPA_ENCRYPTION;
   BSSID[curItem] = isBSSID;
   WPAPersonalKey[curItem] = newKey;

   // Refresh the SSID listbox and enable the apply button
   slotRefreshSSIDList();
   slotCheckGlobalText();
}


void wificonfigwidgetbase::slotAddNewProfileWPAE( QString SSID, bool isBSSID, int type, QString EAPIdent, QString AnonIdent, QString CACert, QString ClientCert, QString PrivKeyFile, QString PrivKeyPass, int keyMgmt, int EAPPhase2 )
{

   for (int dupItem = 0; dupItem < 145; dupItem++)
   {
      if ( SSID == SSIDList[dupItem] ) {
        QMessageBox::warning( this, tr("Error"), tr("You already have a wireless network with this SSID! Please remove it first.\n") );
        return;
      }
   }

   int curItem=0;

   while( ! SSIDList[curItem].isEmpty() && curItem < 145 )
   {
     curItem++;
   }


   SSIDList[curItem] = SSID;
   SSIDEncType[curItem] = WPAE_ENCRYPTION;
   BSSID[curItem] = isBSSID;
   WPAEType[curItem] = type;
   WPAEIdent[curItem] = EAPIdent;
   WPAEAnonIdent[curItem] = AnonIdent;
   WPAECACert[curItem] = CACert;
   WPAEClientCert[curItem] = ClientCert;
   WPAEPrivKeyFile[curItem] = PrivKeyFile;
   WPAEPassword[curItem] = PrivKeyPass;
   WPAEKeyMgmt[curItem] = keyMgmt;
   WPAEPhase2[curItem] = EAPPhase2;

   // Refresh the SSID listbox and enable the apply button
   slotRefreshSSIDList();
   slotCheckGlobalText();

}

void wificonfigwidgetbase::slotOK()
{
    if ( pushApply->isEnabled() )
    {
        slotApply();
	sleep(1);
    }
        close();
}


void wificonfigwidgetbase::slotMACClicked()
{
   if ( checkMAC->isChecked() ) {
	groupMAC->setEnabled(false);
   } else {
	groupMAC->setEnabled(true);
   }
    pushApply->setEnabled(true);
}


QString wificonfigwidgetbase::getLineFromCommandOutput( QString command )
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


QString wificonfigwidgetbase::getNetmaskForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.netmaskAsString();
}


QString wificonfigwidgetbase::getOutErrorsForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.errorsRx());
}


QString wificonfigwidgetbase::getOutPacketsForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.packetsRx());
}


QString wificonfigwidgetbase::getInErrorsForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.errorsTx());
}


QString wificonfigwidgetbase::getInPacketsForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.packetsTx());
}


QString wificonfigwidgetbase::getStatusForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.mediaStatusAsString();
}


QString wificonfigwidgetbase::getMacForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.macAsString();
}


QString wificonfigwidgetbase::getIpForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.ipAsString();
}


QString wificonfigwidgetbase::getNameForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.desc();
}

void wificonfigwidgetbase::showInfo()
{
     tabMainWidget->setCurrentWidget( tabMainWidget->widget(2) );
}


void wificonfigwidgetbase::slotFinishLoading()
{
   QString tmp, tmp2;

/*
   // Look for the mac address change
	if ( tmp.indexOf("ether") != -1 ) {
		tmp2 = tmp;
		tmp2 = tmp2.remove(0, tmp.indexOf("ether") + 6 );
		//if ( tmp.indexOf(" ") != -1 ) {
		//  tmp2.truncate(tmp.indexOf(" ") );
		//}
		lineMAC->setText(tmp2);
		checkMAC->setChecked(false);
		groupMAC->setEnabled(true);
	}
*/

/*
   // Look for a country code
   tmp = trueos::Utils::getConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "=", 1 );
   if ( tmp.indexOf("country ") != -1 ) {
       tmp.remove(0, tmp.lastIndexOf("country") + 8);
       Country = tmp.section(" ", 0,0);
       groupCountryCode->setChecked(true);
   }
*/

   // Refresh the ssid profile list
   slotRefreshSSIDList();

   // Load the list of country codes we can set
   loadCountryCodes();

   pushApply->setEnabled(false);
}


// Slot which reads the arrays and displays the list of WIFI profiles
void wificonfigwidgetbase::slotRefreshSSIDList()
{
    WPAConfiguredNetworks = nmd_status->getWpaNetworkList(DeviceName);
    getWpaNetworks(WPANetworksSystem);
    getWpaNetworks(WPANetworksDialog);
    refreshNetworksList();
}


QString wificonfigwidgetbase::getIPv6ForIdent( QString ident )
{
	QString command = "nice ifconfig " +ident + " | grep \"inet6 \"";
	QString inputLine = getLineFromCommandOutput(command);
	QString ip= "";

	if (inputLine != "" && inputLine.indexOf("inet6 ") != -1){

		ip = inputLine.remove(0, inputLine.indexOf("inet6 ") + 5);
		ip.truncate(ip.indexOf("%"));
	}

	return ip;
}


QString wificonfigwidgetbase::getGatewayForIdent( QString ident )
{
        QString command = "nice netstat -n -r | grep \"default\" | grep " + ident;
	QString inputLine = getLineFromCommandOutput(command);
	QString packets = "";

	if (inputLine != "" ){
		inputLine = inputLine.simplified();
		packets = inputLine.section(' ', 1, 1 );
	}

	return packets;
}


QString wificonfigwidgetbase::getMediaForIdent( QString ident )
{
    	QString command = "nice ifconfig " +ident + " | grep media:";
	QString inputLine = getLineFromCommandOutput(command);
	QString status = "";

	if (inputLine != "" && inputLine.indexOf("media:") != -1){
   	  status = inputLine.remove(0, inputLine.indexOf("media:") + 7);
	}

	return status;
}


void wificonfigwidgetbase::slotCheckGlobalText()
{
    pushApply->setEnabled(true);
}


void wificonfigwidgetbase::setModeWPAOnly()
{
    WPAONLY = true;
    tabMainWidget->removeTab(2);
    tabMainWidget->removeTab(1);
    pushRescan->setHidden(true);
    labelAvailWifi->setHidden(true);
    listNewWifi->setHidden(true);
    pushAddWifi->setHidden(true);
    pushAddHidden->setText(tr("Add &network"));
    setWindowTitle("WPA Supplicant Configuration");
    tabMainWidget->setTabText(0, tr("WPA Configuration") );
}

int wificonfigwidgetbase::getWpaNetworks(QVector<WpaSuppNetwork>& networks)
{
    networks.clear();
    foreach (const auto &net, WPAConfiguredNetworks)
    {
        int id = net.value(QString::fromStdString("NETWORK ID").toLower()).toString().toInt();
        QString ssid = net.value(QString::fromStdString(JSON_PARAM_SSID).toLower()).toString();
        QString bssid = net.value(QString::fromStdString(JSON_PARAM_BSSID).toLower()).toString();
        QString flags = net.value(QString::fromStdString(JSON_PARAM_FLAGS).toLower()).toString();
        bool enabled = !flags.contains(QString::fromStdString("DISABLED"));
        bool current = flags.contains(QString::fromStdString("CURRENT"));
        int priority = net.value(QString::fromStdString("PRIORITY").toLower()).toString().toInt();
        bool protectedd = false;
        QString key_mgmt = net.value(QString::fromStdString("key_mgmt").toLower()).toString();
        if( key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WEP) ||
            key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WPA_PSK) ||
            key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WPA2_PSK)
          )
            protectedd = true;
        else
            protectedd = false;
        WpaSuppNetwork addnet = WpaSuppNetwork(id, ssid, bssid, priority, enabled, current, protectedd);
        networks.push_back(addnet);
    }
    std::sort(networks.begin(), networks.end());
    std::reverse(networks.begin(), networks.end());
    return networks.count();
}

void wificonfigwidgetbase::slotEnableProfile()
{
    if ( listWifi->currentRow() != -1 )
    {
        QString enable_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == enable_ssid )
            {
                WPANetworksDialog[i].Enable();
                break;
            }
        }
        refreshNetworksList();
        pushApply->setEnabled(true);
    }
}

void wificonfigwidgetbase::slotDisableProfile()
{
    if ( listWifi->currentRow() != -1 )
    {
        QString disable_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == disable_ssid )
            {
                WPANetworksDialog[i].Disable();
                break;
            }
        }
        refreshNetworksList();
        pushApply->setEnabled(true);
    }
}
