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
#include "../nmdaemon/nmjsonconst.h"
#include "nmdaemonspy.h"

class NMDAEMONCLIENT_EXPORT NMDaemonClient : public QObject
{

Q_OBJECT

protected:
    static int constexpr DEFAULT_READ_WAIT_TIME_MSEC = 2000;
    static int constexpr SCAN_READ_WAIT_TIME_MSEC = 15000;
    static int constexpr CONNECT_READ_WAIT_TIME_MSEC = 20000;
    static inline const std::string CMD_CONNECT_WIFI_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_CONNECT\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\" } }";
    static inline const std::string CMD_WPA_LIST_NETWORKS = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_SCAN_NETWORKS = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WPA_UPDATE_PSK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SETPSK\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"NETID\" : %d, \"PSK\" : \"%s\" } }";
    static inline const std::string CMD_WPA_ADD_SECURE_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_ADD\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\", \"PSK\" : \"%s\", \"PROFILE\": \"%s\" } }";
    static inline const std::string CMD_WPA_ADD_OPEN_NETWORK = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_ADD\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\", \"SSID\" : \"%s\", \"PROFILE\": \"OPEN\" } }";
    static inline const std::string CMD_WPA_SAVE = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_SAVE\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WIFI_SCAN_RESULTS = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN_RESULTS\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WIFI_SCAN = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const QString WIFI_SECURITY_ERROR = "ERROR";
    static inline const QString WIFI_SECURITY_OPEN = "OPEN";
    static inline const QString WIFI_SECURITY_WEP = "WEP";
    static inline const QString WIFI_SECURITY_WPA_PSK = "WPA-PSK";
    static inline const QString WIFI_SECURITY_WPA2_PSK = "WPA2-PSK";
    static inline const QString NMD_SOCKET_DEFAULT = "/var/run/nmd.socket";
    static constexpr int NM_MAXBUF = 8192;
    static constexpr int SOCKET_CONNECT_TIMEOUT_MSEC = 1000;
    QLocalSocket* nmdSocket;
    QString socketPath;
    bool isPersistentConn;
    bool isConnected;
    char buf[NM_MAXBUF];
    //NMDaemonSpy* spy;
    bool doConnect();
    bool doDisconnect();
    QString lastError;
    QJsonObject lastJsonReceived;
//    QMutex writeSocket;
    QSemaphore writeSocket;
public:
    NMDaemonClient();
    NMDaemonClient(bool);
    NMDaemonClient(QString, bool);
    ~NMDaemonClient();
    bool sendRequest(QJsonObject);
    QString getLastError();
    std::string connectWifiNetwork(QString, QString);
    bool updatePsk(QString, QString, QString, bool);
    bool addNetwork(QString, QString, QString, bool);
    //bool addOpenNetwork(QString, QString);
    bool saveConfig(QString);
    int getWpaNetworkId(QString, QString);
    QJsonObject getJsonForSsid(QString);
    QString getWifiNetworkSecurity(QString, QString);
signals:
    void dataReady(QJsonObject);
    void connected();
    void disconnected();
private slots:
    void slotReadyRead();
    void slotError(QLocalSocket::LocalSocketError);
    void slotConnected();
    void slotDisconnected();
    void slotStatusChanged(QLocalSocket::LocalSocketState);
};

#endif // NMDAEMONCLIENT_H
