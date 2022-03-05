#include "nmdaemonclient.h"

NMDaemonClient::NMDaemonClient() : NMDaemonClient(NMD_SOCKET_DEFAULT, false)
{
}

NMDaemonClient::NMDaemonClient(QString socket_path, bool is_persistent) : socketPath(socket_path),
                                                                          isPersistentConn(is_persistent),
                                                                          isConnected(false)
{
    nmdSocket = new QLocalSocket(this);
    memset(buf, 0, sizeof(buf));
    connect(nmdSocket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred),
            this, &NMDaemonClient::slotError);
    nmdSocket->setServerName(socketPath);
    connect(nmdSocket, &QLocalSocket::connected, this, &NMDaemonClient::slotConnected);
    connect(nmdSocket, &QLocalSocket::disconnected, this, &NMDaemonClient::slotDisconnected);
    connect(nmdSocket, &QLocalSocket::readyRead, this, &NMDaemonClient::slotReadyRead);
}

NMDaemonClient::~NMDaemonClient()
{
    if(isConnected)
        nmdSocket->disconnect();
    delete nmdSocket;
}

void NMDaemonClient::slotReadyRead()
{
    QDataStream in(nmdSocket);
    QJsonObject json_received;
    QString qstr_received = "";
    memset(buf, 0, sizeof(buf));
    for(;;)
    {
        if( (nmdSocket->bytesAvailable() < (int)sizeof(char)) || (!isConnected) )
            break;
        if(in.readRawData(buf, NM_MAXBUF) > 0)
        {
            qstr_received += QString(buf);
            memset(buf, 0, sizeof(buf));
        }
    }
    QJsonDocument qjdoc_received = QJsonDocument::fromJson(qstr_received.toUtf8());
    if(!qjdoc_received.isNull())
    {
        if(qjdoc_received.isObject())
        {
            json_received = qjdoc_received.object();
        }
    }
    if(json_received != lastJsonReceived)
    {
        lastJsonReceived = json_received;
        emit dataReady(json_received);
    }
    if(!isPersistentConn)
        doDisconnect();
}

void NMDaemonClient::slotError(QLocalSocket::LocalSocketError error)
{
    lastError =
        "Error: " + (error == QLocalSocket::ServerNotFoundError ?
                     "The socket " + socketPath + " is not found." :
                     error == QLocalSocket::PeerClosedError ?
                     "The server has closed the connection." :
                     error == QLocalSocket::ConnectionRefusedError ?
                     "The connection was refused (check socket " + socketPath + " permissions)." :
                     QString(nmdSocket->errorString()));
}

bool NMDaemonClient::sendRequest(QJsonObject json_command)
{
    memset(buf, 0, sizeof(buf));
    const char *ptr = nullptr;
    NMDaemonSpy* spy = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
    ptr = str_cmd.c_str();
    strlcpy(buf,ptr,NM_MAXBUF);
    int len = (str_cmd.length()<NM_MAXBUF) ? str_cmd.length() : NM_MAXBUF-1;
    buf[len] = 0;
    if(!isConnected)
    {
        spy = new NMDaemonSpy(nmdSocket, &QLocalSocket::connected);
        doConnect();
        if(!isConnected)
        {
            if(!spy->wait(SOCKET_CONNECT_TIMEOUT_MSEC))
                qDebug() << "Signal QLocalSocket::connected was not received in time";
            if(!isConnected)
            {
                delete spy;
                return false;
            }
        }
        delete spy;
    }
    int len_sent = nmdSocket->write(buf, len+1);
    if(len_sent<len+1)
        return false;
    else
        return true;
}

void NMDaemonClient::slotConnected()
{
    isConnected = true;
    emit connected();
}

void NMDaemonClient::slotDisconnected()
{
    isConnected = false;
    emit disconnected();
}

void NMDaemonClient::slotStatusChanged(QLocalSocket::LocalSocketState socketState)
{
    switch (socketState)
    {
        case QLocalSocket::UnconnectedState:
            isConnected = false;
            break;
        case QLocalSocket::ConnectedState:
            isConnected = true;
            break;
        default:
            break;
    }
}

bool NMDaemonClient::doConnect()
{
    if(isConnected)
        return true;
    nmdSocket->connectToServer();
    return false;
}

bool NMDaemonClient::doDisconnect()
{
    if(!isConnected)
        return true;
    nmdSocket->disconnectFromServer();
    return false;
}

QString NMDaemonClient::getLastError()
{
    return lastError;
}
