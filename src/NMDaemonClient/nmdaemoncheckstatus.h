#ifndef NMDAEMONCHECKSTATUS_H
#define NMDAEMONCHECKSTATUS_H

#include "nmdaemonclient.h"
#include <QTimer>

class NMDaemonCheckStatus : public NMDaemonClient
{
    Q_OBJECT
protected:
//    static long constexpr DEFAULT_REFRESH_TIME_MSEC = 180000;   // 180sec = 3min
    static long constexpr DEFAULT_REFRESH_TIME_MSEC = 60000;   // 60sec = 1min
    static inline const QString CMD_GET_WPA_IF_LIST = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST_IF\" }";
    static inline const QString CMD_GET_SYS_IF_STATUS = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"IF_LIST\" }";
    static inline const std::string CMD_WIFI_SCAN = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_SCAN\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_WIFI_STATUS = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"WIFI_STATUS\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    long refreshTime;   // msec
    QJsonArray lastIfStatus;
    QVector<QPair<QString,QJsonObject>> lastWifiNetStatus;  // <ifname,status>
    QVector<QPair<QString,QJsonObject>> lastWifiIfStatus;  // <ifname,status>
    QVector<QString> wifiInterfaces;
    bool isMonitoring;
//    bool isWaitingForData;
    QTimer *timerMonitoring;
    NMDaemonSpy* spy;
public slots:
        void updateStatus();
        void qqdebug();
public:
    NMDaemonCheckStatus(int);  // msec
    NMDaemonCheckStatus();
    ~NMDaemonCheckStatus();
    void startMonitoring();
    void stopMonitoring();
    void setInterval(int);
    QJsonArray getLastIfStatus() const { return lastIfStatus; };
    QVector<QPair<QString,QJsonObject>> getLastWifiNetStatus() const { return lastWifiNetStatus; };
    QVector<QPair<QString,QJsonObject>> getLastWifiIfStatus() const { return lastWifiIfStatus; };
    QString getIfMainAddress(QString) const;
    QPair<QString,int> getIfWifiNetwork(QString ifname) const;  // <SSID,power>
    QVector<QPair<QString,int>> getNetsWifiPower(QString) const;
    QString getIfStatusString(QString) const;
    QPair<QString,QString> getIfMedia(QString) const;   // Status, Media
    signals:
        void wifiIfListUpdated();
        void ifStatusUpdated();
        void wifiNetStatusUpdated();
        void wifiIfStatusUpdated();
};

#endif // NMDAEMONCHECKSTATUS_H
