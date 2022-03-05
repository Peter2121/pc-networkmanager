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
#include "nmdaemonspy.h"

class NMDAEMONCLIENT_EXPORT NMDaemonClient : public QObject
{

Q_OBJECT

protected:
    static inline const QString NMD_SOCKET_DEFAULT = "/var/run/nmd.socket";
    static constexpr int NM_MAXBUF = 8192;
    static constexpr int SOCKET_CONNECT_TIMEOUT_MSEC = 1000;
    QLocalSocket* nmdSocket;
    QString socketPath;
    bool isPersistentConn;
    bool isConnected;
    char buf[NM_MAXBUF];
    bool doConnect();
    bool doDisconnect();
    QString lastError;
    QJsonObject lastJsonReceived;
public:
    NMDaemonClient();
    NMDaemonClient(QString, bool);
    ~NMDaemonClient();
    bool sendRequest(QJsonObject);
    QString getLastError();
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
