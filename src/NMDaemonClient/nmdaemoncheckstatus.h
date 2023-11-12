#ifndef NMDAEMONCHECKSTATUS_H
#define NMDAEMONCHECKSTATUS_H

#include "nmdaemonclient.h"
#include <QTimer>

struct WifiNetworkDetails
{
    QString SSID;
    QString Security;
    int Power;
};

class NMDaemonCheckStatus : public NMDaemonClient
{
    Q_OBJECT
protected:
//    static long constexpr REFRESH_TIME_DEFAULT_MSEC = 180000;   // 180sec = 3min
    static long constexpr REFRESH_TIME_DEFAULT_MSEC = 60000;   // 60sec = 1min
    static inline const QString CMD_GET_WPA_IF_LIST = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST_IF\" }";
    static inline const QString CMD_GET_SYS_IF_STATUS = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"IF_LIST\" }";
    static inline const std::string CMD_WIFI_SCAN = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const QString CMD_GET_SYS_JAIL_LIST = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"JAIL_LIST\" }";
    long refreshTime;   // msec
    QJsonArray lastIfStatus;
    QJsonArray lastJailStatus;
    QVector<QPair<QString,QJsonObject>> lastWifiNetStatus;  // <ifname,status>
    QVector<QPair<QString,QJsonObject>> lastWifiIfStatus;  // <ifname,status>
    QVector<QString> wifiInterfaces;
    bool isMonitoring;
//    bool isWaitingForData;
    QTimer *timerMonitoring;
    NMDaemonSpy* spy;
public slots:
        void UpdateStatus();
        void SlotStartMonitoring();
public:
    NMDaemonCheckStatus(int, QString);  // msec
    NMDaemonCheckStatus(QString);  // msec
    NMDaemonCheckStatus(int);  // msec
    NMDaemonCheckStatus();
    ~NMDaemonCheckStatus();
    void StartMonitoring();
    void StopMonitoring();
    void SetInterval(int);
    bool UpdateStatusInterfaces();
    bool UpdateStatusJails();
    bool UpdateStatusWifi();
    QJsonArray getLastIfStatus() const { return lastIfStatus; }
    QJsonArray getLastJailStatus() const { return lastJailStatus; }
    QVector<QPair<QString,QJsonObject>> getLastWifiNetStatus() const { return lastWifiNetStatus; }
    QVector<QPair<QString,QJsonObject>> getLastWifiIfStatus() const { return lastWifiIfStatus; }
    QPair<QString,int> GetLastIfWifiNetwork(QString ifname) const;  // <SSID,power>
    QVector<QPair<QString,int>> GetLastNetsWifiPower(QString) const;
    QVector<WifiNetworkDetails> GetLastNetsWifiDetails(QString) const;
    QString GetLastIfStatusString(QString) const;
    QPair<QString,QString> getIfMedia(QString) const;   // Status, Media
    std::shared_ptr<QVector<QString>> GetLastIPv4AliasesFromInterface(QString);
    std::unique_ptr<QVector<QString>> GetLastJailsIPv4();
    std::unique_ptr<QVector<QString>> GetLastJailsIPv6();
    signals:
        void wifiIfListUpdated();
        void ifStatusUpdated();
        void jailStatusUpdated();
        void wifiNetStatusUpdated();
        void wifiIfStatusUpdated();
};

#endif // NMDAEMONCHECKSTATUS_H
