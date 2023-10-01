#ifndef NMDAEMONCLIENT_H
#define NMDAEMONCLIENT_H

#include "NMDaemonClient_global.h"
#include <QString>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDataStream>
#include <QSignalSpy>
//#include <QMutex>
#include <QSemaphore>
#include <QtConcurrent>
#include <functional>
#include <unistd.h>
#include <sockpp/unix_connector.h>
#include <sockpp/version.h>
#include "../nmdaemon/nmjsonconst.h"
#include "nmdaemonspy.h"

class NMDAEMONCLIENT_EXPORT NMDaemonClient : public QObject
{

Q_OBJECT

protected:
    static inline const std::string CMD_CONNECT_WIFI_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_CONNECT\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\" } }";
    static inline const std::string CMD_WPA_LIST_NETWORKS = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_SCAN_NETWORKS = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_UPDATE_PSK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPSK\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PSK\" : \"%s\" } }";
    static inline const std::string CMD_WPA_ADD_SECURE_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_ADD\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\", \"PSK\" : \"%s\", \"PROFILE\": \"%s\" } }";
    static inline const std::string CMD_WPA_ADD_OPEN_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_ADD\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\", \"PROFILE\": \"OPEN\" } }";
    static inline const std::string CMD_WPA_SAVE = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SAVE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_ENABLE_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPARAM\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PARAM_NAME\" : \"disabled\", \"PARAM_VALUE\" : \"0\", \"PARAM_QUOTES\" : false } }";
    static inline const std::string CMD_WPA_DISABLE_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPARAM\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PARAM_NAME\" : \"disabled\", \"PARAM_VALUE\" : \"1\", \"PARAM_QUOTES\" : false } }";
    static inline const std::string CMD_WPA_REMOVE_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_REMOVE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d } }";
    static inline const std::string CMD_WIFI_SCAN_RESULTS = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN_RESULTS\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WIFI_SCAN = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_GET_IPV4_ADDRESS = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP4_ADDR_GET\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_GET_IPV4_DHCP_STATUS = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP4_GET_DHCP_STATUS\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_SET_IPV4_ADDRESS = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP_ADDR_SET\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"IPV4 ADDRESS\" : \"%s\", \"IPV4 SUBNET MASK\" : \"%s\" } }";
    static inline const std::string CMD_ADD_IPV4_ADDRESS = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP_ADDR_ADD\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"IPV4 ADDRESS\" : \"%s\", \"IPV4 SUBNET MASK\" : \"%s\" } }";
    static inline const std::string CMD_REMOVE_IPV4_ADDRESS = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP_ADDR_REMOVE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"IPV4 ADDRESS\" : \"%s\", \"IPV4 SUBNET MASK\" : \"%s\" } }";
    static inline const std::string CMD_SET_IF_ENABLE = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"IF_ENABLE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_SET_IF_DISABLE = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"IF_DISABLE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_SET_IPV4_DHCP_ENABLE = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP4_DHCP_ENABLE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_SET_IPV4_DHCP_DISABLE = "{ \"SCOPE\" : \"INTERFACE\", \"CMD\" : \"IP4_DHCP_DISABLE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_SET_NETWORK_PRI = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPARAM\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PARAM_NAME\" : \"priority\", \"PARAM_VALUE\" : \"%d\", \"PARAM_QUOTES\" : false } }";
    static inline const std::string CMD_WPA_SET_NETWORK_SSID = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPARAM\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PARAM_NAME\" : \"ssid\", \"PARAM_VALUE\" : \"%s\", \"PARAM_QUOTES\" : true } }";
    static inline const std::string CMD_WPA_SET_NETWORK_BSSID = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SET_BSSID\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"BSSID\" : \"%s\" } }";

    //static inline const QString CMD_GET_WPA_IF_LIST = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST_IF\" }";
    //static inline const QString CMD_GWPA_TSAV = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SAVE\", \"DATA\" : { \"INTERFACE NAME\" : \"wlan0\" } }";
    static inline const QString NMD_SOCKET_DEFAULT = "/var/run/nmd.socket";
    static constexpr int NM_MAXBUF = 8192;
    static constexpr int SOCKET_CONNECT_TIMEOUT_MSEC = 500;
    static constexpr std::chrono::milliseconds WAIT_TIME = std::chrono::milliseconds(200);
    static constexpr int WAIT_CYCLES = 10;
    //QLocalSocket* nmdSocket;
    sockpp::unix_connector* nmdUnixSocket;
    QString socketPath;
    bool isPersistentConn;
    bool isConnected;
    char buf[NM_MAXBUF];
    //NMDaemonSpy* spy;
    bool doConnect();
    bool doDisconnect();
    void readData();
    QString lastError;
    QJsonObject lastJsonReceived;
//    QMutex writeSocket;
    QSemaphore writeSocket;
    QString checkDataResultSource;
    std::function<void()> callbackFunctionSuccess;
    std::function<void()> callbackFunctionError;
    void cbWifiConnect(QString, QString);
    void cbWifiNetEnable(QString, QString);
    void cbWifiNetAdd(QString, QString);
    void cbSendSuccessWifiNetEnable(QString ssid) { emit successWifiNetEnable(ssid); }
    void cbSendSuccessWifiNetAdd(QString ssid, int id) { emit successWifiNetAdd(ssid,id); }
public:
    static int constexpr DEFAULT_READ_WAIT_TIME_MSEC = 5000;
    static int constexpr SCAN_READ_WAIT_TIME_MSEC = 15000;
    static int constexpr CONNECT_READ_WAIT_TIME_MSEC = 20000;
    static inline const QString WIFI_SECURITY_ERROR = "ERROR";
    static inline const QString WIFI_SECURITY_OPEN = "OPEN";
    static inline const QString WIFI_SECURITY_WEP = "WEP";
    static inline const QString WIFI_SECURITY_WPA_PSK = "WPA-PSK";
    static inline const QString WIFI_SECURITY_WPA2_PSK = "WPA2-PSK";

    NMDaemonClient();
    NMDaemonClient(bool);
    NMDaemonClient(QString, bool);
    ~NMDaemonClient();
    bool sendRequest(QJsonObject);
    QString getLastError();
    bool connectWifiNetwork(QString, QString);
    bool UpdatePsk(QString, QString, QString, bool);
    bool UpdatePsk(QString, int, QString, bool);
    bool addNetwork(QString, QString, QString, bool);
    bool addNetwork(QString, QString, QString, QString, bool);
    //bool addOpenNetwork(QString, QString);
    bool saveConfig(QString);
    //bool tsavConfigg(QString);
    bool enableNetwork(QString, QString);
    int getWpaNetworkId(QString, QString);
    QJsonObject getJsonForSsid(QString);
    QString getWifiNetworkSecurity(QString, QString) const;
    bool checkSuccess(QJsonObject);
    QString getIPv4Address(QString);
    bool isDhcp4Enabled(QString);
    bool setIPv4Address(QString, QString, QString);
    bool addIPv4Address(QString, QString, QString);
    bool removeIPv4Address(QString, QString, QString);
    bool enableInterface(QString);
    bool disableInterface(QString);
    bool sendCommand(QString, QString);
    bool enableDhcp4(QString);
    bool disableDhcp4(QString);
    QVector<QJsonObject> getWpaNetworkList(QString);
    bool SetNetworkEnabled(QString, int, bool);
    bool SetNetworkPriority(QString, int, int);
    bool RemoveNetwork(QString, int);
    bool SetNetworkSsid(QString, int, QString);
    bool SetNetworkBssid(QString, int, QString);
signals:
    void dataReady(QJsonObject);
    void connected();
    void disconnected();
    void dataReceived();
    void dataError(QString, QString);
    void dataSuccess(QString, QString);
    void successWifiConnect(QString);
    void errorWifiConnect(QString, QString);
    void successWifiNetEnable(QString);
    void errorWifiNetEnable(QString, QString);
    void successWifiNetAdd(QString,int);
    void errorWifiNetAdd(QString, QString);
private slots:
    void slotDataReceived();
    void slotCheckDataResult(QJsonObject);
};

#endif // NMDAEMONCLIENT_H
