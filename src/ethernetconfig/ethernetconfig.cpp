/***************************************************************************
 *   Copyright (C) 2007 by Kris Moore   *
 *   kris@trueos.com   *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/

#include <iostream>
#include <QCheckBox>
#include <QDesktopWidget>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
/*
#include <trueos-netif.h>
#include <trueos-utils.h>
*/
#include "../Utils/utils.h"
#include "ethernetconfig.h"
#include "ui_ethernetconfig.h"

#include "ipv4addrinput.h"
#include "ui_ipv4addrinput.h"

using namespace std;

void ethernetconfig::slot_close()
{
    if(IsMain)
      exit(0);
    else
      this->close();
}

// Function to check if we have wlan devices on this system
/*
bool ethernetconfig::isWifiEnabled() {
	QString command = "ifconfig -l";
	QString inputLine = getLineFromCommandOutput(command);
	if ( inputLine.indexOf("wlan") != -1 )
		return true;
	else
		return false;
}
*/

void ethernetconfig::slot_apply()
{

    QString ifConfigLine;

    if ( ! buttonApply->isEnabled() )
	return;

    if ( checkDisableNetwork->isChecked() )
    {
      if( changedUpDown )
      {
      if(nmd_status->disableInterface(DeviceName))
         buttonApply->setEnabled(false);
      return;
      }
    }
    else
    {
        if( changedUpDown )
        {
        if(nmd_status->enableInterface(DeviceName))
           buttonApply->setEnabled(false);
        }
    }
    // Apply IPv4 aliases
    auto ui_aliases = getIPv4AliasesFromUi();
    auto if_aliases = nmd_status->getLastIPv4AliasesFromInterface(DeviceName);
    std::unique_ptr<QVector<QString>> aliases_to_add = std::make_unique<QVector<QString>>(0);
    std::unique_ptr<QVector<QString>> aliases_to_remove = std::make_unique<QVector<QString>>(0);
    foreach(const QString alias, *ui_aliases)
    {
        if(!if_aliases->contains(alias))
            aliases_to_add->push_back(alias);
    }
    foreach(const QString alias, *if_aliases)
    {
        if(!ui_aliases->contains(alias))
            aliases_to_remove->push_back(alias);
    }
    foreach(const QString alias, *aliases_to_add)
    {
        qDebug() << "Adding alias " << alias;
        nmd_status->addIPv4Address(DeviceName, alias.split("/").value(0).trimmed(), alias.split("/").value(1).trimmed());
    }
    foreach(const QString alias, *aliases_to_remove)
    {
        qDebug() << "Removing alias " << alias;
        nmd_status->removeIPv4Address(DeviceName, alias.split("/").value(0).trimmed(), alias.split("/").value(1).trimmed());
    }
    if ( groupStatic->isChecked() )
    {
        if ( lineIP->text() == "..." || lineNetmask->text() == "..." )
        {
            QMessageBox::information( this, tr("Missing Fields"),
                        tr("You must enter an IP and Netmask to continue!\n") );
            return;
        }
	
        if ( ! Utils::validateIPv4(lineIP->text() ) )
        {
            QMessageBox::warning( this, tr("Warning"),
            tr("IP Address is out of range! (") + lineIP->text() + tr(") Fields must be between 0-255.") );
            return;
        }

        if ( ! Utils::validateIPv4(lineNetmask->text() ) )
        {
            QMessageBox::warning( this, tr("Warning"),
            tr("Netmask is out of range! (") + lineNetmask->text() + tr(") Fields must be between 0-255.") );
            return;
        }
/*
	if ( groupWPAE->isChecked() )
	  ifConfigLine="WPA ";

	if ( checkMAC->isChecked() ) 
	    ifConfigLine+=lineIP->text() + " netmask " + lineNetmask->text();
	else
	    ifConfigLine+=lineIP->text() \
                        +" netmask " + lineNetmask->text() \
                        + " ether " + lineMAC->text();
*/
        if( nmd_status->setIPv4Address(DeviceName, lineIP->text(), lineNetmask->text()) )
            buttonApply->setEnabled(false);
    }
/*
    else {
	// Using WPAE config?
	if ( groupWPAE->isChecked() )
	  ifConfigLine="WPA DHCP";
	else
	  ifConfigLine="DHCP";
    }
*/
    if ( checkIPv6Enable->isChecked() ) {
       // Get rid of the ifconfig line for the ipv6 interface
       // Remove old deprecated syntax as well:
       //trueos::Utils::setConfFileValue( "/etc/rc.conf", "ipv6_ifconfig_" + DeviceName + "=", "", -1);
       //trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "_ipv6=", "", -1);
       // Set accept_rtadv
       //trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "_ipv6=", "ifconfig_" + DeviceName + "_ipv6=\"inet6 accept_rtadv\"", -1);
    } else {
       // Set static address and disbale accept_rtadv.  This means someone will also have to set
       // the default gateway which currently is configured form the advanced tab.  This should
       // move here or all should move to a dedicated IPv6 tab like "General" is IPv4ish.
       if (lineIPv6Address->text() != "") {
	  QString tmp = lineIPv6Address->text();
          if (!tmp.contains(QRegExp("^inet6 "))) {
	     tmp.prepend("inet6 ");
          }
          //trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "_ipv6=", "ifconfig_" + DeviceName + "_ipv6=\"" + tmp + " -accept_rtadv\"", -1);
       }
    }

   //trueos::Utils::setConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "=", "ifconfig_" + DeviceName + "=\"" + ifConfigLine + "\"", -1);
/*
   // See if we need to enable lagg on this device
   if ( isWifiEnabled() && useLagg )
      NetworkInterface::enableLagg(QString("wlan0"));
    
   trueos::Utils::restartNetworking();
*/
   // Done, now set the apply button to off
   //buttonApply->setEnabled(false);
}

void ethernetconfig::slotIPCheckbox()
{
   // Done, now set the apply button to on
   buttonApply->setEnabled(true);
}

void ethernetconfig::programInit()
{
   // See if we need to enable / disable the IP setup
   //slotIPCheckbox();
   // Done, now set the apply button to on
   buttonApply->setEnabled(false);

   if(DeviceName.isEmpty())
   {
       qDebug() << "Device name not set!";
       return;
   }

   // Connect our buttons / slots
   connect(buttonClose, SIGNAL(clicked()), this, SLOT(slot_close()) );
   connect(buttonApply, SIGNAL(clicked()), this, SLOT(slot_apply()) );
   connect(btnRemoveAlias, SIGNAL(clicked()), this, SLOT(slotRemoveAlias()) );
   connect(btnAddAlias, SIGNAL(clicked()), this, SLOT(slotAddAlias()) );
   connect(checkDisableNetwork, SIGNAL(clicked()), this, SLOT(slotDisableCheck()) );
   connect(groupStatic, SIGNAL(clicked()), this, SLOT(slotIPCheckbox()) );
   connect(groupWPAE, SIGNAL(clicked()), this, SLOT(slotIPCheckbox()) );
   connect(buttonOK, SIGNAL(clicked()), this, SLOT(slotOK()) );
   connect(checkMAC, SIGNAL(clicked()), this, SLOT(slotMacClicked()) );
   connect(lineIP, SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckGlobalText() ) );
   connect(lineNetmask, SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckGlobalText() ) );
   connect(checkIPv6Enable, SIGNAL(clicked()), this, SLOT(slotIPv6clicked() ) );
   connect(lineIPv6Address, SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckGlobalText() ) );
   connect(lineMAC, SIGNAL(textChanged(const QString &)), this, SLOT(slotCheckGlobalText() ) );
   connect(pushWPAE, SIGNAL(clicked()), this, SLOT(slotConfigWPA()) );
   connect(nmd_status, SIGNAL(ifStatusUpdated()), this, SLOT(slotFinishLoad()));

   nmd_status->updateStatusInterfaces();
   /*
   NMDaemonSpy* spy = new NMDaemonSpy(nmd_status, &NMDaemonCheckStatus::ifStatusUpdated);
   nmd_status->startMonitoring();
   if(!spy->wait(10000))
       qDebug() << "Signal NMDaemonCheckStatus::ifStatusUpdated was not received in time";
   delete spy;
   nmd_status->stopMonitoring();
   */
//   QTimer::singleShot(30,  this,  SLOT(loadInfo()));
//   QTimer::singleShot(30,  this,  SLOT(slotFinishLoad()));
}

void ethernetconfig::setDevice(QString Device)
{
   
   buttonApply->setEnabled(false);

   DeviceName = Device;

   this->setWindowTitle(DeviceName + " " + tr("configuration") );

   // Connect the slot to load the values
   //QTimer::singleShot(100,  this,  SLOT(slotFinishLoad()));
}

void ethernetconfig::runCommand( QString command )
{
	FILE *file = popen(command.toLatin1(),"r"); 
	pclose(file);
}


void ethernetconfig::slotDisableCheck()
{
  if ( checkDisableNetwork->isChecked() )
  {
	tabMainWidget->setEnabled(false);
  } else {
	tabMainWidget->setEnabled(true);
  }
   // Done, now set the apply button to on
   buttonApply->setEnabled(true);
   changedUpDown = true;
}

void ethernetconfig::slotOK()
{
  slot_apply();
  slot_close();
}

void ethernetconfig::slotIPv6clicked()
{
	if ( checkIPv6Enable->isChecked() ) {
		groupIPv6->setEnabled(false);
	} else {
		groupIPv6->setEnabled(true);
	}

   // Done, now set the apply button to on
   buttonApply->setEnabled(true);
}

void ethernetconfig::slotMacClicked()
{
   if ( checkMAC->isChecked() ) {
	groupMAC->setEnabled(false);
   } else {
	groupMAC->setEnabled(true);
   }
   // Done, now set the apply button to on
   buttonApply->setEnabled(true);
}

void ethernetconfig::loadInfo()
{
  QString tmp;
  QJsonValue json_value;
  QJsonObject json_object,json_obj;
  QJsonArray json_array;
  QString str_value,str_type;
  QString dev;

  QJsonArray if_status = nmd_status->getLastIfStatus();
  if(!if_status.empty())
  {
      foreach (const QJsonValue &v, if_status)
      {
          json_object = v.toObject();
          json_value = json_object.value("INTERFACE NAME");
          dev = json_value.toString();
          if(dev != DeviceName)
              continue;
          json_value = json_object.value("MEDIA");
          str_value = json_value.toString();
          if(!str_value.contains("Ethernet"))
          {
               qDebug() << "Unknown device media: " << str_value;
               return;
          }
          str_value = json_object.value("STATUS").toString();
          textStatus->setText(str_value);
          str_value = json_object.value("MEDIA").toString();
          textMedia->setText(str_value);
          json_array = json_object.value("ADDRESSES").toArray();
          textIP->setText("");
          foreach(auto addr, json_array)
          {
              json_obj = addr.toObject();
              str_type = json_obj.value("ADDRESS TYPE").toString();
              // TODO: add support for other types of addresses
              str_value = json_obj.value(str_type).toObject().value("IPV4 ADDRESS").toString();
              if(!str_value.isEmpty())
              {
                  if(textIP->text().isEmpty())
                      textIP->setText(str_value);
                  else
                      textIP->setText(textIP->text()+"\n"+str_value);
                  str_value = json_obj.value(str_type).toObject().value("IPV4 SUBNET MASK").toString();
                  if(!str_value.isEmpty())
                    textNetmask->setText(str_value);
              }
              str_value = json_obj.value(str_type).toObject().value("LINK ADDRESS").toString();
              if(!str_value.isEmpty())
              {
                  textMac->setText(str_value);
              }
              str_value = json_obj.value(str_type).toObject().value("IPV6 ADDRESS").toString();
              if(!str_value.isEmpty())
              {
                  textIPv6->setText(str_value);
              }

          }

      }
  }

  // Get all the device information for the info tab
  //if ( textInfoName->text().isEmpty() ) {
  //tmp = getNameForIdent(DeviceName);
  //tmp.truncate(25);
  //textInfoName->setText(tmp);
  //}

  //QString fDev = DeviceName ;
  //if ( useLagg )
  //  fDev = "lagg0" ;

  //textIP->setText(getIpForIdent(fDev) );
  //textNetmask->setText(getNetmaskForIdent(fDev) );
  //textStatus->setText(getStatusForIdent( DeviceName ) );
/*
  textMac->setText(getMacForIdent( DeviceName ) );
  textIPv6->setText(getIPv6ForIdent( DeviceName) );
  textGateway->setText(getGatewayForIdent(fDev) );
*/
  //tmp = getMediaForIdent(DeviceName);
  //tmp.truncate(20);
  //textMedia->setText(tmp);

  // Get the packet status for this device
  //textPacketsIn->setText(getInPacketsForIdent( DeviceName) );
  //textPacketsInErrors->setText( getInErrorsForIdent( DeviceName) );
  //textPacketsOut->setText(getOutPacketsForIdent( DeviceName) );
  //textPacketsOutErrors->setText(getOutErrorsForIdent( DeviceName) );

  // Connect the slot to refresh
  //QTimer::singleShot(3000,  this,  SLOT(loadInfo()));
}
/*
QString ethernetconfig::getNameForIdent( QString ident )
{
    NetworkInterface ifr(ident);
    return ifr.desc();
}

QString ethernetconfig::getIpForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.ipAsString();
}

QString ethernetconfig::getMacForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.macAsString();
}

QString ethernetconfig::getStatusForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.mediaStatusAsString();
}

QString ethernetconfig::getInPacketsForIdent(QString ident)
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.packetsRx());
}

QString ethernetconfig::getInErrorsForIdent(QString ident)
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.errorsRx());
}

QString ethernetconfig::getOutPacketsForIdent(QString ident)
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.packetsTx());
}

QString ethernetconfig::getOutErrorsForIdent(QString ident)
{
   NetworkInterface ifr(ident);
   return QString::number(ifr.errorsTx());
}

QString ethernetconfig::getNetmaskForIdent( QString ident )
{
   NetworkInterface ifr(ident);
   return ifr.netmaskAsString();
}
*/
QString ethernetconfig::getLineFromCommandOutput( QString command )
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

void ethernetconfig::showInfo()
{
	tabMainWidget->setCurrentWidget( tabMainWidget->widget(2) );
}

void ethernetconfig::slotFinishLoad()
{
   QString tmp, tmp2;
   QJsonValue json_value;
   QJsonObject json_object, json_obj;
   QJsonArray json_array;
   QString str_value,str_type;
   QString dev;
   //QSettings settings("PCBSD");
   //useLagg = settings.value("/pc-netmanager/useLagg", false).toBool();

   // Start loading the device information
   //if ( useLagg )
   //   tmp = trueos::Utils::getConfFileValue( "/etc/rc.conf", "ifconfig_lagg0=", 1 );

   //if ( tmp.isEmpty() )
   //   tmp = trueos::Utils::getConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "=", 1 );

   // Using WPAE config?
   //if ( tmp.indexOf("WPA") != -1 )
   //   groupWPAE->setChecked(true);

   checkIPv6Enable->setChecked(true);   // TODO: Check it in rc.conf
   groupIPv6->setEnabled(false);
   QJsonArray if_status = nmd_status->getLastIfStatus();
   QString primary_ipv4 = nmd_status->getIPv4Address(DeviceName);
   /*
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_object = json_value.toObject();
            if(!json_object.empty())
            {
                json_value = json_object.value(QString::fromStdString(JSON_DATA_DHCP_STATUS));
                str_status = json_value.toString();
                if(str_status.toStdString()==JSON_DATA_ENABLED)
                    status = true;
            }
    */

   //bool dhcp_enabled = nmd_status->isDhcpEnabled(DeviceName);
   bool dhcp_enabled = false;
   bool is_up = false;
   bool is_running = false;
   if(!if_status.empty())
   {
       foreach (const QJsonValue &v, if_status)
       {
           json_object = v.toObject();
           json_value = json_object.value(QString::fromStdString(JSON_PARAM_IF_NAME));
           dev = json_value.toString();
           if(dev != DeviceName)
               continue;
           json_value = json_object.value(QString::fromStdString(JSON_PARAM_MEDIA));
           str_value = json_value.toString();
           if(!str_value.contains("Ethernet"))
           {
                qDebug() << "Unknown device media: " << str_value;
                return;
           }
           json_value = json_object.value(QString::fromStdString(JSON_PARAM_DHCP_ENABLED));
           if(json_value.isString() && (json_value.toString().toStdString()==JSON_DATA_ENABLED) )
               dhcp_enabled = true;
           if(dhcp_enabled)
               groupStatic->setChecked(false);
           else
               groupStatic->setChecked(true);
           json_value = json_object.value(QString::fromStdString(JSON_PARAM_UP));
           if(json_value.isBool())
               is_up = json_value.toBool();
           json_value = json_object.value(QString::fromStdString(JSON_PARAM_RUNNING));
           if(json_value.isBool())
               is_running = json_value.toBool();
           if(is_up && is_running)
           {
               checkDisableNetwork->setChecked(false);
               tabMainWidget->setEnabled(true);
           }
           else
           {
               checkDisableNetwork->setChecked(true);
               tabMainWidget->setEnabled(false);
           }
           json_array = json_object.value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
           foreach(auto addr, json_array)
           {
               json_obj = addr.toObject();
               str_type = json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_TYPE)).toString();
               // TODO: add support for other types of addresses
               str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_ADDR)).toString();
               if(!str_value.isEmpty())
               {
                   if(str_value == primary_ipv4)
                   {
                       lineIP->setText(str_value);
                       str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_MASK)).toString();
                       lineNetmask->setText(str_value);
                   }
                   else
                   {
                       str_value += " / " + json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_MASK)).toString();
                       listAliases->addItem(str_value);
                   }
               }
               /*
               str_value = json_obj.value(str_type).toObject().value("IPV6 ADDRESS").toString();
               if(!str_value.isEmpty())
               {
                   textIPv6->setText(str_value);
               }
               */
           }
       }
    }
/*
   if ( tmp.isEmpty() ) {
      checkDisableNetwork->setChecked(true);
      tabMainWidget->setEnabled(false);
   } else if ( tmp.indexOf("DHCP") != -1 ) {

      groupStatic->setChecked(false);
      lineNetmask->setText("255.255.255.0");

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

   } else {
        // Look for an IP configuration
        groupStatic->setChecked(true);
        tmp2 = tmp;

	// Using the lagg port
    //if ( useLagg ) {
    //    	tmp2.remove(0, tmp2.lastIndexOf("laggport") + 9);
    //    	tmp2 = tmp2.simplified();
    //    	tmp2.remove(0, tmp2.indexOf(" "));
    //}

       	tmp2 = tmp2.simplified(); 

	// Remove any WPA line
	tmp2.replace("WPA ", "");

       	tmp2.truncate(tmp2.indexOf(" "));

      	if ( ! tmp2.isEmpty())
	{
	   lineIP->setText(tmp2);

	   // Look for the netmask
	   if ( tmp.indexOf("netmask") != -1 ) {
			tmp2 = tmp;
			tmp2 = tmp2.simplified();
			tmp2 = tmp2.remove(0, tmp.indexOf("netmask") + 8 );
  			if ( tmp.indexOf("ether") != -1 ) {
			tmp2.truncate(tmp.indexOf("ether") );
			}
			lineNetmask->setText(tmp2);
		} else {
		   lineNetmask->setText("255.255.255.0");
		}

		// Look for the mac address change
		if ( tmp.indexOf("ether") != -1 ) {
			tmp2 = tmp;
			tmp2 = tmp2.remove(0, tmp.indexOf("ether") + 6 );
                        tmp2 = tmp2.simplified();
			lineMAC->setText(tmp2);
			checkMAC->setChecked(false);
			groupMAC->setEnabled(true);
		} 

	}


   }
*/
/*
  // Check for the IPv6 Configuration
  // Start loading the device information
   tmp = trueos::Utils::getConfFileValue( "/etc/rc.conf", "ifconfig_" + DeviceName + "_ipv6=", 1 );
   if ( tmp.isEmpty() ) {
	// Check for backward compat.
	tmp = trueos::Utils::getConfFileValue( "/etc/rc.conf", "ipv6_ifconfig_" + DeviceName + "=", 1 );
   }
   if ( ! tmp.isEmpty() ) {
	// remove the SLAAC default.
	tmp.remove(QRegExp("inet6 "));
        tmp.remove(QRegExp("accept_rtadv"));
        tmp.remove(QRegExp("^\\s*"));
        tmp.remove(QRegExp("\\s*$"));
   }
   if ( tmp.isEmpty() ) {
        checkIPv6Enable->setChecked(true);
   } else {
	checkIPv6Enable->setChecked(false);
	lineIPv6Address->setText(tmp);
   }

  slotIPv6clicked();
*/
  // Starts grabbing information for the info tab
  loadInfo();


  buttonApply->setEnabled(false);
}
/*
QString ethernetconfig::getIPv6ForIdent(QString ident)
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

QString ethernetconfig::getGatewayForIdent(QString ident)
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

QString ethernetconfig::getMediaForIdent( QString ident )
{
    	QString command = "nice ifconfig " +ident + " | grep \"media:\"";
	QString inputLine = getLineFromCommandOutput(command);
	QString status = "";
	
	if (inputLine != "" && inputLine.indexOf("media:") != -1){
		status = inputLine.remove(0, inputLine.indexOf("media:") + 7);
		status.truncate(inputLine.indexOf("\n") );
                if ( status.indexOf("(") != -1)
                {
                   status.remove(0, status.indexOf("(") + 1);
                }
                if ( status.indexOf(")") != -1)
                {
                   status.truncate(status.indexOf(")"));
                }

	}
	
	return status;
}
*/
void ethernetconfig::slotCheckGlobalText()
{
  // Done, now set the apply button to off
  buttonApply->setEnabled(true);
}

void ethernetconfig::slotConfigWPA()
{
   // Bring up the WPA-Enterprise config dialog
   QProcess::execute("/usr/local/bin/pc-wificonfig", QStringList() << "wpaonly" << DeviceName);
}

void ethernetconfig::setIsMain(bool is_main)
{
    IsMain = is_main;
}

void ethernetconfig::slotRemoveAlias()
{
    QListWidgetItem *it = listAliases->currentItem();
    listAliases->removeItemWidget(it);
    delete it;
    buttonApply->setEnabled(true);
}

void ethernetconfig::slotAddAlias()
{
    Ipv4addrinput *ain = new Ipv4addrinput();
    const QString title = "Add IPv4 alias to " + DeviceName;
    ain->setWindowTitle(title);
    connect(ain, SIGNAL( saved(QString,QString) ), this, SLOT( slotAddAliasToList(QString,QString) ) );
    ain->exec();
}

void ethernetconfig::slotAddAliasToList(QString addr, QString mask)
{
    QString str_value = addr + " / " + mask;
    listAliases->addItem(str_value);
    buttonApply->setEnabled(true);
}

std::shared_ptr<QVector<QString>> ethernetconfig::getIPv4AliasesFromUi()
{
    std::shared_ptr<QVector<QString>> ret_aliases = std::make_shared<QVector<QString>>(0);
    for(int row = 0; row < listAliases->count(); row++)
    {
             ret_aliases->push_back(listAliases->item(row)->text());
    }
    return ret_aliases;
}
