// QT Includes
#include <qpair.h>
#include <QProcess>
#include <QDialog>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>
#include <QMenu>

#include "netKey.h"
#include "../backend/backend-network.h"
#include "../NMDaemonClient/nmdaemoncheckstatus.h"
#include "../ethernetconfig/ethernetconfig.h"
#include "../wificonfig/wificonfigwidgetbase.h"

//#define PROGPATH QString("/usr/local/share/trueos/pc-netmanager")
#define PROGPATH QString("../../../pc-networkmanager")
#define ETHCONFIGPATH QString("../ethernetconfig")
#define WIFICONFIGPATH QString("../wificonfig")
#define IFCONFIG QString("/sbin/ifconfig")
#define USE_SVG = 1

class QString;
class QPixmap;

class NetworkTray : public QDialog {
   Q_OBJECT

private:
    static inline const QString IF_WIFI_PREFIX = "wlan";
    static constexpr short UPDATE_MSEC = 5000;
    static constexpr short WIFI_CONNECT_MAX_RETRIES = 5;
    static constexpr short WIFI_CONNECT_RETRY_MSEC = 3000;
    static constexpr int IDLE_REFRESH_TIME_MSEC = 180000;
    static constexpr int ACTIVE_REFRESH_TIME_MSEC = 10000;
    static inline const std::string CMD_CONNECT_WIFI_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_CONNECT\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\" } }";
    static inline const std::string CMD_RESET_WPA = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_RESET\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";

public:
   NetworkTray(NMDaemonCheckStatus* nmds);
   ~NetworkTray();
   void programInit(QString);
   
private slots:
//   void openNetManager();
//   void openDeviceInfo();
   void onActivated(QSystemTrayIcon::ActivationReason);
   void openConfigDlg();
   void resetWpaSupplicant();
//   void monitorStatus(bool noloop=false);
   void updateStatus();
   void setIdleScanTimeout();
   void setActiveScanTimeout();
   void scanWifi();
//   void slotRestartNetwork();
//   void slotTrayActivated(QSystemTrayIcon::ActivationReason);
   void slotQuit();
   void slotSave();
   void slotGetNetKey(QAction* act);
   void slotConnect();
   void slotUpdatePskAndConnect(QString key, QString SSID, bool hexkey);
   void slotAddNetworkAndConnect(QString key, QString ssid, bool hexkey);
//   void slotQuickConnect(QString key, QString SSID, bool hexkey);
//   void slotCheckWifiAvailability();
//   void slotUpdateIfStatus();
//   void slotToggleTorMode(bool enable);
   void slotShowIfStatus();
   void slotShowError(QString, QString);
   void slotWifiConnectError(QString, QString);
   void slotWifiConnectSuccess(QString);
   void slotWifiEnableError(QString, QString);
   void slotWifiAddError(QString, QString);
   void slotWifiConnectNetwork(QString ssid);

protected:
   static constexpr short NUM_SEPARATORS = 3;
   NMDaemonCheckStatus* nmdStatus;
   QAction* QuitAction;
   QAction* SaveAction;
   QAction* ConfigAction;
   QAction* ResetAction;
   QAction* ScanAction;
   QAction* StatusAction;
   QVector<QAction*> ConnectActions;
   QAction* SeparatorActions[NUM_SEPARATORS];
   QSystemTrayIcon *TrayIcon;
   QMenu *TrayIconMenu;
   QIcon IconWiredConnect;  // nm-adhoc
   QIcon IconWiredDisconnect;   // nm-no-connection
   QIcon IconWifiConnect;   // nm-signal-100
   QIcon IconWifiDisconnect;    // nm-signal-00
   QIcon IconWifiConnect25; // nm-signal-25
   QIcon IconWifiConnect50; // nm-signal-50
   QIcon IconWifiConnect75; // nm-signal-75
   QIcon IconLocked;
   QIcon IconUnlocked;
   QIcon IconConfigure; // "preferences-system"
   QIcon IconQuit;  // "application-exit"
   QIcon IconReset;
   QIcon IconScan;
   QIcon IconSave;
   QDateTime LastUpdateTime;
private:
   backend::NetDevice *DEVICE;
   QString IfName;
   bool IsWireless;
   QString LastWifiNetwork;
   int LastWifiPower;
   int numWifiConnectRetries;
//   void displayTooltip();
   void loadIcons();
   void connectWifiNetwork(QString);
//   void updateWifiNetworks();
   QFileSystemWatcher *fileWatcherClosed;
//   QString getLineFromCommandOutput( QString command );
//   QString getSSIDForIdent();
//   QString getSignalStrengthForIdent( QString ident );
//   QString getWirelessSpeedForIdent( QString ident );
//   QString getMediaForIdent();
   void setTrayIcon();
   QIcon getWifiIcon(int power);
   QString getWifiStatusLine(QPair<QString,int>, bool);

//    bool checkTorMode();

   QString ifconfigOutput;
   QProcess *runCommandProc;
   QProcess *getIfProc;
   QActionGroup *trayActionGroup;
   
   netKey *dialogNetKey;
};
