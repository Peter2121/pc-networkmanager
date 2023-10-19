#ifndef WIFICONFIGWIDGETBASE_H
#define WIFICONFIGWIDGETBASE_H

#include <qfile.h>
#include <qmessagebox.h>
#include <qdialog.h>
#include <qwidget.h>
#include <trueos-netif.h>
#include <trueos-utils.h>
#include <trueos-ui.h>
//#include "wifiselectiondialog.h"
//#include "wepconfig.h"
//#include "dialogwpapersonal.h"
#include "ui_wificonfigwidgetbase.h"
#include <QProcess>
#include "../NMDaemonClient/nmdaemoncheckstatus.h"
#include "../NMDaemonClient/nmdaemonspy.h"
#include "wpasuppnetwork.h"
#include "wifiedit.h"
#include "qlistwidgetitemnet.h"
#include "qlistwidgetitemsec.h"


// Setup our encryption type defines
#define NO_ENCRYPTION 0
#define WEP_ENCRYPTION 1
#define WPA_ENCRYPTION 2
#define WPAE_ENCRYPTION 3

#define KEYWPAEAP 0
#define KEY8021X 1

// Setup WPAE Phase2 defines
#define KEYPHASE2MD5 0
#define KEYPHASE2MSCHAPV2 1
#define KEYPHASE2GTC 2
#define KEYPHASE2OTP 3
#define KEYPHASE2PAP 4
#define KEYPHASE2CHAP 5
#define KEYPHASE2MSCHAP 6

class wificonfigwidgetbase : public QWidget, private Ui::wificonfigwidgetbase
{
        Q_OBJECT

public:
        wificonfigwidgetbase();
        wificonfigwidgetbase(NMDaemonCheckStatus*);
        ~wificonfigwidgetbase();

    void setDevice( QString Device );
    void showInfo();
    void setIsMain(bool);
    // Are we just editing WPA supplicant? 
    void setModeWPAOnly();
    void autoConnectSSID(QString ssid);

public slots:
    void slotProfileSaved();

private slots:
    void slotClose();
    void slotApply();
    void slotAddHiddenProfile();
    void slotAddNewProfile();
    void slotEditProfile();
    void slotRemoveProfile();
    void slotEnableProfile();
    void slotDisableProfile();
    void slotRemoveProfileSSID(QString RemoveSSID);
    void slotAddNewProfileSSID(QString,QString,bool,QString);
    void slotAddNewProfileOpen( QString SSID, bool isBSSID );
    void slotAddNewProfileWEP( QString SSID, bool isBSSID, QString newKey, int newIndex, bool hexkey );
    void slotAddNewProfileWPA( QString SSID, bool isBSSID, QString newKey );
    void slotAddNewProfileWPAE( QString SSID, bool isBSSID, int type, QString EAPIdent, QString AnonIdent, QString CACert, QString ClientCert, QString PrivKeyFile, QString PrivKeyPass, int keyMgmt, int EAPPhase2 );
    void slotOK();
    void slotMACClicked();
    void slotFinishLoading();
    void slotCheckGlobalText();
    void SlotStartRefreshSSIDList();
    void slotMoveUp();
    void slotMoveDown();
    void slotFinishRescan();
    void slotWEPSave(QString newkey, int newIndex, bool hexkey );
    void slotWPAPSave(QString newkey);
    void slotItemDoubleClicked(QListWidgetItem*);
    void slotStartRescan();
    void SlotContinueApply(QString,int);
    void SlotFinishRefreshSSIDList(std::shared_ptr<QVector<QJsonObject>>);

private:
    QString getLineFromCommandOutput( QString command );
    QString getNetmaskForIdent( QString ident );
    QString getOutErrorsForIdent( QString ident );
    QString getOutPacketsForIdent( QString ident );
    QString getInErrorsForIdent( QString ident );
    QString getInPacketsForIdent( QString ident );
    QString getStatusForIdent( QString ident );
    QString getMacForIdent( QString ident );
    QString getIpForIdent( QString ident );
    QString getNameForIdent( QString ident );
    QString getIPv6ForIdent( QString ident );
    QString getGatewayForIdent( QString ident );
    QString getMediaForIdent( QString ident );
    QString getWifiParent( QString dev );
    int getWpaNetworks(QVector<std::shared_ptr<WpaSuppNetwork>>& nets);
    void loadCountryCodes();
    void runCommand( QString Command );
    //void updateWPASupp();
    QString DeviceName;
    QString Country;
    bool IsMain;
    bool OwnNmdStatus;
    NMDaemonCheckStatus* nmd_status;
    QVector<QJsonObject> WPAConfiguredNetworks;
    QVector<std::shared_ptr<WpaSuppNetwork>> WPANetworksSystem;
    QVector<std::shared_ptr<WpaSuppNetwork>> WPANetworksDialog;
    void refreshNetworksList();
    void EditProfile(int);
    QString GetNetSecurity(QString, QString);

    QProcess *netifProc;

    // Lets define our arrays for the SSID profiles
    QString SSIDList[150];
    int SSIDEncType[150];
    bool BSSID[150];
    QString WEPKey[150];
    int WEPIndex[150];
    bool WEPHex[150];
    QString WPAPersonalKey[150];
    QString WPAEIdent[150];
    QString WPAEAnonIdent[150];
    QString WPAECACert[150];
    QString WPAEClientCert[150];
    QString WPAEPrivKeyFile[150];
    QString WPAEPassword[150];
    int WPAEKeyMgmt[150];
    int WPAEPhase2[150];
    int WPAEType[150];
    bool usingLagg;
    bool WPAONLY;

signals:
  void connectSSID(QString); //internal signal for launching the connection dialog for an SSID
  void ApplyConfig();
} ;
#endif // WIFICONFIGWIDGETBASE_H
