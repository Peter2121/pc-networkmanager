#include "nmdaemonworker.h"

NMDaemonWorker::NMDaemonWorker(sockpp::stream_socket socket, char* buf, int buflen) : QObject(nullptr)
{
    NmdUnixSocket = std::move(socket);
    Buf = buf;
    BufLen = buflen;
}

void NMDaemonWorker::ReadData()
{
    int n = 0;
    //char buf[8192];
    //memset(buf, 0, sizeof(buf));
    size_t bufsize = BufLen*sizeof(char);
    memset(Buf, 0, bufsize);
    n = NmdUnixSocket.read_n(Buf, bufsize);
    qDebug() << "Received from socket: " << Buf;
    if(n>0)
    {
        emit DataRead();
        emit Finished();
    }
    else
    {
        QString last_error = QString::fromStdString(NmdUnixSocket.last_error_str());
        qDebug() << "ERROR: Cannot read data from socket: " << last_error;
        emit DataError(last_error);
        emit Finished();
    }
}
