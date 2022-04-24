#include "nmdaemonclient.h"

NMDaemonClient::NMDaemonClient() : NMDaemonClient(NMD_SOCKET_DEFAULT, false)
{
}

NMDaemonClient::NMDaemonClient(bool is_persistent) : NMDaemonClient(NMD_SOCKET_DEFAULT, is_persistent)
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
    std::unique_ptr<NMDaemonSpy> connspy = nullptr;
    //NMDaemonSpy* connspy = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
    ptr = str_cmd.c_str();
    strlcpy(buf,ptr,NM_MAXBUF);
    int len = (str_cmd.length()<NM_MAXBUF) ? str_cmd.length() : NM_MAXBUF-1;
    buf[len] = 0;
    if(!isConnected)
    {
        //connspy = new NMDaemonSpy(nmdSocket, &QLocalSocket::connected);
        connspy = std::make_unique<NMDaemonSpy>(nmdSocket, &QLocalSocket::connected);
        doConnect();
        if(!isConnected)
        {
            if(!connspy->wait(SOCKET_CONNECT_TIMEOUT_MSEC))
                qDebug() << "Signal QLocalSocket::connected was not received in time";
            if(!isConnected)
            {
                return false;
            }
        }
    }
    int len_sent = nmdSocket->write(buf, len+1);
    if(len_sent<len+1)
        return false;
    else
        return true;
}

std::string NMDaemonClient::connectWifiNetwork(QString ifname, QString ssid)
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    QString conf_ssid;
    QString req_string;
    QJsonObject req_object;
    QString flags;
    //bool found = false;
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);

    // Get list of available networks from WPA, search for the ssid in this list and get network parameters
/*
    req_string = QString::asprintf(CMD_WPA_SCAN_NETWORKS.c_str(), ifname.toStdString().c_str());
    req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    delete readspy;
    readspy = new NMDaemonSpy(this, &NMDaemonClient::dataReady);
    if(!sendRequest(req_object))
    {
        delete readspy;
        return JSON_PARAM_ERR;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_object = json_value.toObject();
            json_value = json_object.value(QString::fromStdString(JSON_PARAM_NETWORKS));
            json_array = json_value.toArray();
            found = false;
            foreach (const QJsonValue &v, json_array)
            {
                json_object = v.toObject();
                conf_ssid = json_object.value("ssid").toString();
                if(conf_ssid == ssid)
                {
                    found = true;
                    flags = json_object.value("flags").toString();
                    break;
                }
            }
        }
        else
        {
            delete readspy;
            return JSON_PARAM_ERR;
        }
        if(!found)
        {
            delete readspy;
            return JSON_DATA_ERR_NOT_AVAILABLE;
        }
    }
    else
    {
        delete readspy;
        return JSON_PARAM_ERR;
    }
*/
    // Finally, try to connect and return the result
    req_string = QString::asprintf(CMD_CONNECT_WIFI_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str());
    req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    if(!sendRequest(req_object))
    {
        return JSON_PARAM_ERR;
    }
    readspy->wait(CONNECT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
/*
{ { JSON_PARAM_RESULT, JSON_DATA_ERR_AUTH_FAILED } }
{ { JSON_PARAM_RESULT, JSON_DATA_ERR_NOT_AVAILABLE } }
...
*/
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        return json_value.toString().toStdString();
    }
    else
    {
        return JSON_PARAM_ERR;
    }
}

int NMDaemonClient::getWifiNetworkId(QString ifname, QString ssid)
// Get list of configured networks from WPA and search for the ssid in this list
{
    QString req_string = QString::asprintf(CMD_WPA_LIST_NETWORKS.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    QString conf_ssid;
    int net_id = -1;

    if(!sendRequest(req_object))
    {
        return net_id;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_object = json_value.toObject();
            json_value = json_object.value(QString::fromStdString(JSON_PARAM_NETWORKS));
            json_array = json_value.toArray();
            foreach (const QJsonValue &v, json_array)
            {
                json_object = v.toObject();
                conf_ssid = json_object.value("ssid").toString();
                if(conf_ssid == ssid)
                {
                    net_id = json_object.value("network id").toString().toInt();
                    break;
                }
            }
        }
    }
    return net_id;
}

bool NMDaemonClient::updatePsk(QString ifname, QString ssid, QString psk, bool hexkey)
{
    if(hexkey)
    {
        qDebug() << "Hex keys are not (yet) supported";
        return false;
    }
    int net_id = getWifiNetworkId(ifname, ssid);
    if(net_id<0)
        return false;

    QJsonValue json_value;
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QString req_string = QString::asprintf(CMD_WPA_UPDATE_PSK.c_str(), ifname.toStdString().c_str(), net_id, psk.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    if(!sendRequest(req_object))
    {
        return false;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
            return saveConfig(ifname);
    }
    return false;
}

bool NMDaemonClient::saveConfig(QString ifname)
{
    QJsonValue json_value;
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QString req_string = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    if(!sendRequest(req_object))
    {
        return false;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
            return true;
    }
    return false;
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
