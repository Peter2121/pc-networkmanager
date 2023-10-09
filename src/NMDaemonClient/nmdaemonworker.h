#ifndef NMDAEMONWORKER_H
#define NMDAEMONWORKER_H

#include <QObject>
#include <QDebug>
#include <sockpp/unix_connector.h>
#include <sockpp/version.h>

class NMDaemonWorker : public QObject
{
    Q_OBJECT
public:
    NMDaemonWorker(sockpp::stream_socket, char*, int);
public slots:
    void ReadData();
protected:
    sockpp::stream_socket NmdUnixSocket;
    char *Buf;
    int BufLen;
signals:
    void DataRead();
    void DataError(const QString&);
    void Finished();
};

#endif // NMDAEMONWORKER_H
