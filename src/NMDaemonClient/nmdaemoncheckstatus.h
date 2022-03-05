#ifndef NMDAEMONCHECKSTATUS_H
#define NMDAEMONCHECKSTATUS_H

#include "nmdaemonclient.h"
#include <QTimer>

class NMDaemonCheckStatus : public NMDaemonClient
{
    Q_OBJECT
protected:
    static long constexpr DEFAULT_REFRESH_TIME_MSEC = 300000;
    static int constexpr DEFAULT_READ_WAIT_TIME_MSEC = 2000;
    static inline const QString CMD_GET_IF_LIST = "{ \"SCOPE\" : \"WPA\", \"CMD\" : \"WPA_LIST_IF\" }";
    static inline const QString CMD_GET_IF_STATUS = "{ \"SCOPE\" : \"SYSTEM\", \"CMD\" : \"IF_LIST\" }";
    static inline const std::string CMD_GET_WIFI_NET_STATUS = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"NET_LIST\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    static inline const std::string CMD_GET_WIFI_IF_STATUS = "{ \"SCOPE\" : \"WIFI\", \"CMD\" : \"IF_STATUS\", \"DATA\" : { \"INTERFACE NAME\" : \"%s\" } }";
    long refreshTime;   // msec
    QJsonArray lastIfStatus;
    QVector<QPair<QString,QJsonObject>> lastWifiNetStatus;  // <ifname,status>
    QVector<QPair<QString,QJsonObject>> lastWifiIfStatus;  // <ifname,status>
    QVector<QString> wifiInterfaces;
    NMDaemonSpy* spy;
    bool isMonitoring;
    bool isWaitingForData;
    QTimer *timerMonitoring;
public slots:
        void updateStatus();
        void qqdebug();
public:
    NMDaemonCheckStatus(long);  // msec
    NMDaemonCheckStatus();
    ~NMDaemonCheckStatus();
    void startMonitoring();
    void stopMonitoring();
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
