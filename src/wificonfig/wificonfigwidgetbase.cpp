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
    IsMain = false;
    qRegisterMetaType<QSharedPointer<QVector<QJsonObject>>>();
}

wificonfigwidgetbase::wificonfigwidgetbase(NMDaemonCheckStatus* nmd) : QWidget()
{
    setupUi(this);
    WPAONLY=false;
    nmd_status = nmd;
    OwnNmdStatus = false;
    IsMain = false;
}

wificonfigwidgetbase::~wificonfigwidgetbase()
{
    if(WPANetworksDialog.length()>0)
        WPANetworksDialog.squeeze();

    if(WPANetworksSystem.length()>0)
        WPANetworksSystem.squeeze();
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

void wificonfigwidgetbase::SlotContinueApply(QString ssid, int netid)
{
    int imaxdial = WPANetworksDialog.length();
    connect(this, SIGNAL(ApplyConfig()), this, SLOT(slotApply()) );

    for(int idial=0; idial<imaxdial; idial++)
    {
      if(ssid==WPANetworksDialog[idial]->GetSsid())
      {
          WPANetworksDialog[idial]->SetId(netid);
          WPANetworksDialog[idial]->SetKey("");
          auto newsysnet = std::make_shared<WpaSuppNetwork>(netid, ssid, "any", 0, "false", "false", WPANetworksDialog[idial]->IsProtected());
          WPANetworksSystem.push_back(newsysnet);
          break;
      }
    }
    std::sort(WPANetworksDialog.begin(), WPANetworksDialog.end());
    std::reverse(WPANetworksDialog.begin(), WPANetworksDialog.end());
    refreshNetworksList();
    emit ApplyConfig();
}

void wificonfigwidgetbase::slotApply()
{
    QString tmp;
    QString ifConfigLine;
    QVector<int> IndexToModify;
    QVector<int> IdToRemove;
    //QVector<int> IdToResetPsk;
    QVector<std::tuple<int, QString>> psks; //   netid, psk
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
    int imaxsys = WPANetworksSystem.length();
    int imaxdial = WPANetworksDialog.length();
    bool refresh_list = false;
    for(int idial=0; idial<imaxdial; idial++)
    {
        int found = false;
        QString ssid = WPANetworksDialog[idial]->GetSsid();
        for(int isys=0; isys<imaxsys; isys++)
        {
            if(WPANetworksSystem[isys]->GetSsid() == ssid)
            {
                found = true;
                break;
            }
        }
        if(!found)  // New network
        {
            connect( nmd_status, SIGNAL(successWifiNetAdd(QString,int)), this, SLOT(SlotContinueApply(QString,int)), Qt::QueuedConnection );
            if(!nmd_status->AddNetwork(DeviceName, ssid, WPANetworksDialog[idial]->GetKey(), WPANetworksDialog[idial]->GetSecurity(), false))
            {
                QMessageBox::warning(this, "Cannot add network", "Cannot add network "+ssid+" to the system config");
            }
            else
            {
                return;
            }
        }
    }
    // Apply priority, SSID, BSSID and enable/disable
    for(int idial=0; idial<imaxdial; idial++)
    {
        for(int isys=0; isys<imaxsys; isys++)
        {
            if(WPANetworksSystem[isys]->GetId() == WPANetworksDialog[idial]->GetId())
            {
                if( (WPANetworksSystem[isys]->GetPriority() != WPANetworksDialog[idial]->GetPriority()) ||
                     (WPANetworksSystem[isys]->IsEnabled() != WPANetworksDialog[idial]->IsEnabled()) ||
                     (WPANetworksSystem[isys]->GetSsid() != WPANetworksDialog[idial]->GetSsid()) ||
                     (WPANetworksSystem[isys]->GetBssid() != WPANetworksDialog[idial]->GetBssid()) )
                {
                    if( (WPANetworksSystem[isys]->GetSsid() != WPANetworksDialog[idial]->GetSsid()) &&
                        (WPANetworksSystem[isys]->GetPriority() == WPANetworksDialog[idial]->GetPriority()) )
                    {
                        auto list_item = listWifi->item(listWifi->currentRow());    // TODO: What if the selected item is not the one modified??
                        if(list_item!=nullptr)
                        {
                            QString new_text = WPANetworksDialog[idial]->GetSsid();
                            QString new_tip = "ID: "+ QString::number(WPANetworksDialog[idial]->GetId()) + "\nSSID: " + WPANetworksDialog[idial]->GetSsid() + "\nPriority: " + QString::number(WPANetworksDialog[idial]->GetPriority());
                            if(!WPANetworksDialog[idial]->IsEnabled())
                            {
                                new_text += " [disabled]";
                                new_tip += "\n[disabled]";
                            }
                            if(WPANetworksDialog[idial]->IsCurrent())
                            {
                                new_text += " [current]";
                                new_tip += "\n[current]";
                            }
                            list_item->setText(new_text);
                            list_item->setToolTip(new_tip);
                        }
                    }
                    if(WPANetworksSystem[isys]->GetPriority() != WPANetworksDialog[idial]->GetPriority())
                        refresh_list = true;
                    WPANetworksSystem[isys]->SetPriority(WPANetworksDialog[idial]->GetPriority());
                    WPANetworksSystem[isys]->SetSsid(WPANetworksDialog[idial]->GetSsid());
                    WPANetworksSystem[isys]->SetBssid(WPANetworksDialog[idial]->GetBssid());
                    WPANetworksDialog[idial]->IsEnabled() ? WPANetworksSystem[isys]->Enable() : WPANetworksSystem[isys]->Disable();
                    IndexToModify.push_back(isys);
                }
                if (!WPANetworksDialog[idial]->GetKey().isEmpty())
                {
                    psks.push_back(std::make_tuple(WPANetworksDialog[idial]->GetId(), WPANetworksDialog[idial]->GetKey()));
                    WPANetworksDialog[idial]->SetKey(QString());
                }
            }
        }
    }
    foreach (int i, IndexToModify) {
        int netid = WPANetworksSystem[i]->GetId();
        nmd_status -> SetNetworkEnabled(DeviceName, netid, WPANetworksSystem[i]->IsEnabled());
        nmd_status -> SetNetworkPriority(DeviceName, netid, WPANetworksSystem[i]->GetPriority());
        nmd_status -> SetNetworkSsid(DeviceName, netid, WPANetworksSystem[i]->GetSsid());
        nmd_status -> SetNetworkBssid(DeviceName, netid, WPANetworksSystem[i]->GetBssid());
        nmd_status -> SaveConfig(DeviceName);
    }

    // Update keys
    foreach (auto psk, psks) {
        //nmd_status -> UpdatePsk(DeviceName, std::get<0>(psk), std::get<1>(psk), false);
        //nmd_status -> SaveConfig(DeviceName);
        nmd_status -> UpdatePskAndSave(DeviceName, std::get<0>(psk), std::get<1>(psk), false);
    }

    // Apply remove
    foreach (const auto &net, WPANetworksSystem) {
        bool found = false;
        for(int i=0; i<imaxdial; i++)
        {
            if(WPANetworksDialog[i]->GetId() == net->GetId())
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            IdToRemove.push_back(net->GetId());
        }
    }
    foreach (int id, IdToRemove) {
        //nmd_status -> RemoveNetwork(DeviceName, id);
        //nmd_status -> SaveConfig(DeviceName);
        nmd_status -> RemoveNetworkAndSave(DeviceName, id);
    }

    buttonCancel->setEnabled(true);
    pushApply->setEnabled(false);
    if(refresh_list)
    {
        std::sort(WPANetworksDialog.begin(), WPANetworksDialog.end());
        std::reverse(WPANetworksDialog.begin(), WPANetworksDialog.end());
        refreshNetworksList();
    }
}

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
  connect( pushRescan, SIGNAL( clicked() ), this, SLOT( slotStartRescan() ) );
  connect( pushAddHidden, SIGNAL( clicked() ), this, SLOT( slotAddHiddenProfile() ) );
  connect( pushConnect, SIGNAL( clicked() ), this, SLOT( SlotConnect() ) );
  connect( groupCountryCode, SIGNAL( clicked() ), this, SLOT( slotCheckGlobalText() ) );
  connect( listWifi, SIGNAL( itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotItemDoubleClicked(QListWidgetItem*)));
  connect( this, SIGNAL( RefreshWPANetworks()), SLOT( SlotStartRefreshSSIDList() ));
  connect( this, SIGNAL( StartRescan()), SLOT( slotStartRescan() ));
  connect( nmd_status, SIGNAL(dataError(QString, QString)), this, SLOT(slotShowError(QString, QString)));

  DeviceName = Device;
  pushApply->setEnabled(false);

  // Check if we are using the lagg0 device
  //QSettings settings("PCBSD");
  //usingLagg = settings.value("/pc-netmanager/useLagg", false).toBool();

  //loadCountryCodes();

  listWifi->clear();
  listWifi->addItem("Please, wait...");
  listWifi->update();
  QCoreApplication::processEvents();
  emit StartRescan();
  emit RefreshWPANetworks();
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
            if (reader.name() == QString("country")) {
	     name="";
	     code = reader.attributes().value("id").toString();
             while (!reader.atEnd()) {
                    if (reader.isEndElement() && reader.name() == QString("country"))
            break;
                    if (reader.isStartElement() && reader.name() == QString("name")) {
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

void wificonfigwidgetbase::slotStartRescan()
{
    connect(nmd_status, SIGNAL(wifiNetStatusUpdated()), this, SLOT(slotFinishRescan()));
    nmd_status->UpdateStatus();
}

void wificonfigwidgetbase::slotFinishRescan()
{
    QString strength, ssid, security, FileLoad;
    QStringList ifconfout, ifline;
    int foundItem = 0;
    QListWidgetItemSec *net_line = nullptr;

    // Clear the list box and disable the add button
    listNewWifi->clear();
    pushAddWifi->setEnabled(false);
    QVector<WifiNetworkDetails> wifi_networks = nmd_status->GetLastNetsWifiDetails(DeviceName);

    foreach(const auto &net, wifi_networks)
    {
        ssid = net.SSID;
//        strength = net.Power;
        if(net.Security == NMDaemonClient::WIFI_SECURITY_OPEN)
            FileLoad = ":object-unlocked.png";
        else if( (net.Security == NMDaemonClient::WIFI_SECURITY_WEP) ||
                 (net.Security == NMDaemonClient::WIFI_SECURITY_WPA_PSK) ||
                 (net.Security == NMDaemonClient::WIFI_SECURITY_WPA2_PSK) )
            FileLoad = ":object-locked.png";
        if(FileLoad.isEmpty())
            net_line = new QListWidgetItemSec(ssid + " (signal: " + QString(std::to_string(net.Power).c_str()) + "%)");
        else
            net_line = new QListWidgetItemSec(QIcon(FileLoad), ssid + " (signal: " + QString(std::to_string(net.Power).c_str()) + "%)");
        net_line->SetSecurity(net.Security);
//        listNewWifi->addItem(new QListWidgetItem(QIcon(FileLoad), ssid + " (signal: " + QString(std::to_string(net.Power).c_str()) + "%)") );
        listNewWifi->addItem(net_line);
        foundItem = 1;
    }

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
    listWifi->update();
    foreach (const auto &net, WPANetworksDialog)
    {
        QString ssid = net->GetSsid();
        QString net_line = ssid;
        QString net_tip = "ID: " + QString::number(net->GetId()) + "\nSSID: " + ssid + "\nPriority: " + QString::number(net->GetPriority());
        QListWidgetItemNet* item_net_line = nullptr;
        if(!net->IsEnabled())
        {
            net_line += " [disabled]";
            net_tip += "\n[disabled]";
        }
        if(net->IsCurrent())
        {
            net_line += " [current]";
            net_tip += "\n[current]";
        }
        if(net->IsProtected())
        {
            item_net_line = new QListWidgetItemNet(QIcon(":object-locked.png"), net_line);
            item_net_line ->SetNetId(net->GetId());
            item_net_line->setToolTip(net_tip);
            listWifi->addItem(item_net_line);
        }
        else
        {
            item_net_line = new QListWidgetItemNet(QIcon(":object-unlocked.png"), net_line);
            item_net_line ->SetNetId(net->GetId());
            item_net_line->setToolTip(net_tip);
            listWifi->addItem(item_net_line);
        }
    }
    listWifi->update();
    listWifi->setMouseTracking(true);
}


// Slot which moves a SSID higher in the connection priority
void wificonfigwidgetbase::slotMoveUp()
{
    int itomove = -1;

    if ( listWifi->currentRow() != -1 )
    {
        /*
        QString move_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == move_ssid )
            {
                itomove = i;
                break;
            }
        }
        */
        int idnet = ((QListWidgetItemNet*)(listWifi->item(listWifi->currentRow())))->GetNetId();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i]->GetId() == idnet )
            {
                itomove = i;
                break;
            }
        }
        if(itomove >= 1 && (itomove < WPANetworksDialog.count()))
        {
            if(WPANetworksDialog[itomove-1]->GetPriority()==WPANetworksDialog[itomove]->GetPriority())
                WPANetworksDialog[itomove]->IncreasePriority();
            else if(WPANetworksDialog[itomove-1]->GetPriority()>WPANetworksDialog[itomove]->GetPriority())
                WPANetworksDialog[itomove]->SetPriority(WPANetworksDialog[itomove-1]->GetPriority()+1);
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
        /*
        QString move_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i].GetSsid() == move_ssid )
            {
                itomove = i;
                break;
            }
        }
        */
        int idnet = ((QListWidgetItemNet*)(listWifi->item(listWifi->currentRow())))->GetNetId();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i]->GetId() == idnet )
            {
                itomove = i;
                break;
            }
        }
        if( (itomove >= 0) && (itomove < WPANetworksDialog.count()-1))
        {
            if(WPANetworksDialog[itomove+1]->GetPriority()==WPANetworksDialog[itomove]->GetPriority())
                WPANetworksDialog[itomove]->DecreasePriority();
            else if(WPANetworksDialog[itomove+1]->GetPriority()<WPANetworksDialog[itomove]->GetPriority())
                WPANetworksDialog[itomove]->SetPriority(WPANetworksDialog[itomove+1]->GetPriority()-1);
            std::sort(WPANetworksDialog.begin(), WPANetworksDialog.end());
            std::reverse(WPANetworksDialog.begin(), WPANetworksDialog.end());
            refreshNetworksList();
            pushApply->setEnabled(true);
        }
    }
}

void wificonfigwidgetbase::slotItemDoubleClicked(QListWidgetItem* item)
{
    int itoedit = -1;
    int idnet = ((QListWidgetItemNet*)(item))->GetNetId();
    for (int i=0; i<WPANetworksDialog.count(); i++)
    {
        if ( WPANetworksDialog[i]->GetId() == idnet )
        {
            itoedit = i;
            break;
        }
    }
    EditProfile(itoedit);
}

void wificonfigwidgetbase::EditProfile(int itoedit)
{
    std::unique_ptr<WifiEdit> wifi_edit_dialog = nullptr;
    if( (itoedit >= 0) && (itoedit < WPANetworksDialog.count()))
    {
        wifi_edit_dialog = std::make_unique<WifiEdit>(WPANetworksDialog[itoedit].get());
        connect( wifi_edit_dialog.get(), SIGNAL( Saved() ), this, SLOT( slotProfileSaved() ) );
        wifi_edit_dialog->setModal(true);
        wifi_edit_dialog->exec();
    }
}

void wificonfigwidgetbase::slotEditProfile()
{
    int itoedit = -1;
    std::unique_ptr<WifiEdit> wifi_edit_dialog = nullptr;

    // Check if we have a selection to work with
    if ( listWifi->currentRow() != -1 )
    {
        int idnet = ((QListWidgetItemNet*)(listWifi->item(listWifi->currentRow())))->GetNetId();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i]->GetId() == idnet )
            {
                itoedit = i;
                break;
            }
        }
        EditProfile(itoedit);

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
  if( ok && !ssidc.isEmpty())
  {
    netKey *dialogNetKey = new netKey();
    dialogNetKey->setSSID(ssidc);
    dialogNetKey->showKeyType(true);
    dialogNetKey->EnableSecurityCombo();
    connect( dialogNetKey, SIGNAL( saved(QString,QString,bool,QString) ), this, SLOT( slotAddNewProfileSSID(QString,QString,bool,QString) ) );
    dialogNetKey->exec();
  }
}

void wificonfigwidgetbase::slotAddNewProfile()
{
   //Get the selected SSID
   if( (listNewWifi->currentRow())!=-1 )
   {
        QListWidgetItemSec *net = ((QListWidgetItemSec*)(listNewWifi->item(listNewWifi->currentRow())));
        QString line = net->text();
        QString ssidc = line.section(" (",0,0,QString::SectionSkipEmpty);
        QString security = net->GetSecurity();
        foreach (const auto &net, WPANetworksDialog)
        {
            if(net->GetSsid()==ssidc)
            {
                QMessageBox::warning(this, "Network exists", "This network exists already in the configuration");
                return;
            }
        }
        if( (security==NMDaemonClient::WIFI_SECURITY_OPEN) ||
            (security==NMDaemonClient::WIFI_SECURITY_ERROR) )
        {
            auto newnet = std::make_shared<WpaSuppNetwork>(0, ssidc, "any", 0, true, false, false);
            WPANetworksDialog.push_back(newnet);
            refreshNetworksList();
            pushApply->setEnabled(true);
        }
        else if( (security==NMDaemonClient::WIFI_SECURITY_WEP) ||
                 (security==NMDaemonClient::WIFI_SECURITY_WPA_PSK) ||
                 (security==NMDaemonClient::WIFI_SECURITY_WPA2_PSK) )
        {
            netKey *dialogNetKey = new netKey();
            dialogNetKey->setSSID(ssidc);
            dialogNetKey->showKeyType(true);
            dialogNetKey->SetSecurity(security);
            dialogNetKey->SetSecurityCombo();
            connect( dialogNetKey, SIGNAL( saved(QString,QString,bool,QString) ), this, SLOT( slotAddNewProfileSSID(QString,QString,bool,QString) ) );
            dialogNetKey->exec();
        }
   }
   else
   {
        QMessageBox::warning(this, "No Network Selected", "Please select a wireless network to add");
   }
}

void wificonfigwidgetbase::slotAddNewProfileSSID(QString key, QString ssidc, bool use_hex, QString security)
{
   if(use_hex)
   {
        QMessageBox::warning(this, "Not Supported", "Hex keys are not (yet) supported");
        return;
   }
   auto newnet = std::make_shared<WpaSuppNetwork>(0, ssidc, "any", 0, false, false, true);
   newnet->SetKey(key);
   newnet->SetSecurity(security);
   WPANetworksDialog.push_back(newnet);
   refreshNetworksList();
   pushApply->setEnabled(true);

/*
    //Get the Security Type
    QString sec = NetworkInterface::getWifiSecurity(ssidc,DeviceName);

    //Save the SSID for the future save slots
    saveSSID = ssidc;

    //Open the proper security dialog and link it to the save slots
    if(sec.contains("None")){
      slotAddNewProfileOpen(ssidc,false);
    }else if(sec.contains("WEP")){
     wepConfig dialogWEP;
     connect( &dialogWEP, SIGNAL( saved(QString, int, bool) ), this, SLOT( slotWEPSave(QString, int, bool) ) );
     dialogWEP.exec();
    }else if(sec.contains("WPA")){
     dialogWPAPersonal dialogWPA;
     connect( &dialogWPA, SIGNAL( saved(QString) ), this, SLOT( slotWPAPSave(QString) ) );
     dialogWPA.exec();
    }else{
      //Put error handling here
      slotAddNewProfileOpen(ssidc,false);
      if ( ! WPAONLY )
        QMessageBox::warning(this,"Unknown Wifi Security","The security for this wifi access point could not be automatically determined.\n Please edit the profile to manually set the proper security encryption.");
    }
*/
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

/*
void wificonfigwidgetbase::runCommand( QString Command )
{
    FILE *file = popen(Command.toLatin1(),"r");
	pclose(file);
}
*/
// TODO: correctly remove profile when the same ssid is present X times (use netid)
void wificonfigwidgetbase::slotRemoveProfileSSID(QString remove_ssid)
{
    for (int i=0; i<WPANetworksDialog.count(); i++)
    {
        if ( WPANetworksDialog[i]->GetSsid() == remove_ssid )
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
    SlotStartRefreshSSIDList();
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
    SlotStartRefreshSSIDList();
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
   SlotStartRefreshSSIDList();
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
   SlotStartRefreshSSIDList();
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
   SlotStartRefreshSSIDList();

   // Load the list of country codes we can set
   loadCountryCodes();

   pushApply->setEnabled(false);
}


void wificonfigwidgetbase::SlotStartRefreshSSIDList()
{
   connect( nmd_status, SIGNAL( UpdatedWpaNetworkList(QSharedPointer<QVector<QJsonObject>>) ), this, SLOT( SlotFinishRefreshSSIDList(QSharedPointer<QVector<QJsonObject>>) ), Qt::QueuedConnection );
   nmd_status->UpdateWpaNetworkList(DeviceName, true);
}

void wificonfigwidgetbase::SlotFinishRefreshSSIDList(QSharedPointer<QVector<QJsonObject>> wpa_networks)
{
   WPAConfiguredNetworks = *(wpa_networks.get());
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

int wificonfigwidgetbase::getWpaNetworks(QVector<std::shared_ptr<WpaSuppNetwork>>& networks)
{
    networks.squeeze();
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
        QString key_mgmt_proto = net.value(QString::fromStdString("proto").toLower()).toString();
        if( key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WEP) ||
            key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WPA_PSK)
          )
            protectedd = true;
        else
            protectedd = false;
        auto addnet = std::make_shared<WpaSuppNetwork>(id, ssid, bssid, priority, enabled, current, protectedd);
        addnet->SetSecurity(GetNetSecurity(key_mgmt, key_mgmt_proto));
        networks.push_back(addnet);
    }
    std::sort(networks.begin(), networks.end());
    std::reverse(networks.begin(), networks.end());
    return networks.count();
}

QString wificonfigwidgetbase::GetNetSecurity(QString key_mgmt, QString key_mgmt_proto)
{
    if( key_mgmt.contains(NMDaemonClient::WIFI_SECURITY_WPA_PSK) )
    {
        if(key_mgmt_proto.contains("RSN"))
            return NMDaemonClient::WIFI_SECURITY_WPA2_PSK;
        else if(key_mgmt_proto.contains("WPA"))
            return NMDaemonClient::WIFI_SECURITY_WPA_PSK;
        else return NMDaemonClient::WIFI_SECURITY_ERROR;
    }
    else
    {
        return key_mgmt;
    }
}

void wificonfigwidgetbase::slotEnableProfile()
{
    if ( listWifi->currentRow() != -1 )
    {
        QString enable_ssid = listWifi->item(listWifi->currentRow())->text().split('[')[0].trimmed();
        for (int i=0; i<WPANetworksDialog.count(); i++)
        {
            if ( WPANetworksDialog[i]->GetSsid() == enable_ssid )
            {
                WPANetworksDialog[i]->Enable();
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
            if ( WPANetworksDialog[i]->GetSsid() == disable_ssid )
            {
                WPANetworksDialog[i]->Disable();
                break;
            }
        }
        refreshNetworksList();
        pushApply->setEnabled(true);
    }
}

void wificonfigwidgetbase::SlotConnect()
{
    int netid = -1;
    if ( listWifi->currentRow() != -1 )
    {
        netid = ((QListWidgetItemNet*)(listWifi->item(listWifi->currentRow())))->GetNetId();
    }
    if(netid>=0)
    {
    connect(nmd_status, SIGNAL(successWifiConnect(int)), this, SLOT(SlotConnectSuccess(int)));
    connect(nmd_status, SIGNAL(errorWifiConnect(int, QString)), this, SLOT(SlotConnectError(int, QString)));
    nmd_status->ConnectWifiNetwork(DeviceName, netid);
    }
}

void wificonfigwidgetbase::SlotConnectSuccess(int)
{
    SlotStartRefreshSSIDList();
}

void wificonfigwidgetbase::SlotConnectError(int netid, QString)
{
    QMessageBox::information(this, QString("ERROR"), QString("Cannot connect to network ") + QString(netid));
}

void wificonfigwidgetbase::slotShowError(QString source, QString message)
{
    QMessageBox::critical(this, "Error from "+source, message);
}
