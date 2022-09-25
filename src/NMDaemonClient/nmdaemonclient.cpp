#include "nmdaemonclient.h"

NMDaemonClient::NMDaemonClient() : NMDaemonClient(NMD_SOCKET_DEFAULT, false)
{
}

NMDaemonClient::NMDaemonClient(bool is_persistent) : NMDaemonClient(NMD_SOCKET_DEFAULT, is_persistent)
{
}

NMDaemonClient::NMDaemonClient(QString socket_path, bool is_persistent) : socketPath(socket_path),
                                                                          isPersistentConn(is_persistent),
                                                                          isConnected(false),
                                                                          writeSocket(1)
{
    nmdUnixSocket = new sockpp::unix_connector();
    memset(buf, 0, sizeof(buf));
    callbackFunction = nullptr;
//    connect(nmdSocket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred),
//            this, &NMDaemonClient::slotError);
//    connect(nmdSocket, &QLocalSocket::connected, this, &NMDaemonClient::slotConnected);
//    connect(nmdSocket, &QLocalSocket::disconnected, this, &NMDaemonClient::slotDisconnected);
    connect(this, &NMDaemonClient::dataReceived, this, &NMDaemonClient::slotDataReceived);
}

NMDaemonClient::~NMDaemonClient()
{
    doDisconnect();
    delete nmdUnixSocket;
}

bool NMDaemonClient::doConnect()
{
    if(isConnected)
        return true;
    bool ok = nmdUnixSocket->connect(sockpp::unix_address(socketPath.toStdString()));
    if(!ok)
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        qDebug() << "Cannot connect to " << socketPath << " socket:\n" << lastError;
        return false;
    }
    isConnected = true;
    emit connected();
    return true;
}

bool NMDaemonClient::doDisconnect()
{
    if(!isConnected)
        return true;
    if(!nmdUnixSocket->close())
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        return false;
    }
    isConnected = false;
    emit disconnected();
    return true;
}

QString NMDaemonClient::getLastError()
{
    return lastError;
}

void NMDaemonClient::slotDataReceived()
{
    QJsonObject json_received;
    QString qstr_received = QString(buf);
    //qDebug() << "Received from socket: " << qstr_received;
    QJsonDocument qjdoc_received = QJsonDocument::fromJson(qstr_received.toUtf8());
    if(!qjdoc_received.isNull())
    {
        if(qjdoc_received.isObject())
        {
            json_received = qjdoc_received.object();
        }
    }
    if(!isPersistentConn)
        doDisconnect();
    if(writeSocket.available() == 0)
        writeSocket.release();
    if(!json_received.empty() && (json_received != lastJsonReceived) )
    {
        lastJsonReceived = json_received;
        emit dataReady(json_received);
    }
}

/*
bool NMDaemonClient::sendRequest(QJsonObject json_command)
{
    memset(buf, 0, sizeof(buf));
    QDataStream in(nmdSocket);
    QString qstr_received = "";
    const char *ptr = nullptr;
    std::unique_ptr<NMDaemonSpy> connspy = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();    
    if(isConnected)
    {
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
        if(!qstr_received.isEmpty())
            qDebug() << "Unattended data received in sendRequest: " << qstr_received;
    }
    else
    {
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
    ptr = str_cmd.c_str();
    strlcpy(buf,ptr,NM_MAXBUF);
    int len = (str_cmd.length()<NM_MAXBUF) ? str_cmd.length() : NM_MAXBUF-1;
    buf[len] = 0;
    if(writeSocket.available()>0)
        writeSocket.acquire();
    else
    {
        bool available=false;
        for(int i=0; i<10; i++)
        {
            sleep(1);
            if(writeSocket.available()>0)
            {
                available=true;
                break;
            }
        }
        if(available)
            writeSocket.acquire();
        else
        {
            qDebug() << "FATAL: Cannot acquire write lock on nmdaemon socket";
            exit(1);
        }
    }
    lastJsonReceived = QJsonObject();
    nmdSocket->flush();
//    connect(nmdSocket, &QLocalSocket::readyRead, this, &NMDaemonClient::slotReadyRead);
    qDebug() << "Writing to socket: " << buf;
    int len_sent = nmdSocket->write(buf, len+1);
    //nmdSocket->flush();
    nmdSocket->waitForBytesWritten();
    nmdSocket->flush();
    //sleep(1);
    if(len_sent<len+1)
    {
        writeSocket.release();
        return false;
    }
    else
        return true;
}
*/
bool NMDaemonClient::sendRequest(QJsonObject json_command)
{
    QString qstr_received = "";
    const char *ptr = nullptr;
    //std::unique_ptr<NMDaemonSpy> connspy = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
    /*
    if(isConnected)
    {
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
        if(!qstr_received.isEmpty())
            qDebug() << "Unattended data received in sendRequest: " << qstr_received;
    }
    else
    {
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
    */
    if(!doConnect())
        return false;
    if(writeSocket.available()>0)
        writeSocket.acquire();
    else
    {
        bool available = false;
        for(int i=0; i<10; i++)
        {
            sleep(1);
            if(writeSocket.available()>0)
            {
                available=true;
                break;
            }
        }
        if(available)
            writeSocket.acquire();
        else
        {
            qDebug() << "FATAL: Cannot acquire write lock on nmdaemon socket trying to write: " << QString::fromStdString(str_cmd);
            exit(1);
        }
    }
    lastJsonReceived = QJsonObject();
    memset(buf, 0, sizeof(buf));
    ptr = str_cmd.c_str();
    strlcpy(buf,ptr,NM_MAXBUF);
    int len = (str_cmd.length()<NM_MAXBUF) ? str_cmd.length() : NM_MAXBUF-1;
    buf[len] = 0;
    qDebug() << "Writing to socket: " << buf;
    int len_sent = nmdUnixSocket->write(buf, sizeof(buf));
    if(len_sent<len+1)
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        qDebug() << "Error writing to socket: " << lastError;
        writeSocket.release();
        return false;
    }
    else
    {
        //QtConcurrent::run(this, &NMDaemonClient::readData);   //  Data lost in socket communications due to race conditions
        //******************************
        //Writing to socket:  {"CMD":"WPA_SETPARAM","DATA":{"INTERFACE NAME":"wlan0","NETID":11,"PARAM_NAME":"disabled","PARAM_QUOTES":false,"PARAM_VALUE":"0"},"SCOPE":"WPA"}
        //Received from socket:  {"RESULT":"SUCCESS"}
        //FATAL: Cannot acquire write lock on nmdaemon socket trying to write:  "{\"CMD\":\"IF_LIST\",\"SCOPE\":\"SYSTEM\"}"
        //******************************
        readData();    // TODO: call it in a separate thread ??
        return true;
    }
}

void NMDaemonClient::readData()
{
    int n = 0;
    if(!isConnected)
    {
        qDebug() << "ERROR: readData from disconnected socket";
        return;
    }
    memset(buf, 0, sizeof(buf));
    n = nmdUnixSocket->read_n(buf, sizeof(buf));
    qDebug() << "Received from socket: " << buf;
    if(n>0)
        emit dataReceived();
    else
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        qDebug() << "ERROR: Cannot read data from socket: " << lastError;
    }
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

QJsonObject NMDaemonClient::getJsonForSsid(QString ssid)
// Get JSON object from lastJsonReceived for given SSID
{
    QJsonArray json_array;
    QJsonValue json_value;
    QJsonObject json_object;
    QJsonObject ret_object;
    QString conf_ssid;

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
            /*
            if(!json_object.value("ssid").Undefined)
                conf_ssid = json_object.value("ssid").toString();
            else if(!json_object.value("SSID").Undefined)
                conf_ssid = json_object.value("SSID").toString();
            else
                conf_ssid = ""; // TODO: manage this case better
            */
            conf_ssid = json_object.value("ssid").toString();
            if(conf_ssid.isEmpty())
                conf_ssid = json_object.value("SSID").toString();
            if(conf_ssid == ssid)
            {
                ret_object = json_object;
                break;
            }
        }
    }
    return ret_object;
}

bool NMDaemonClient::enableNetwork(QString ifname, QString ssid)
{
    int net_id = getWpaNetworkId(ifname, ssid);
    if(net_id<0)
        return false;
    QJsonValue json_value;
    //std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QString req_string = QString::asprintf(CMD_WPA_ENABLE_NETWORK.c_str(), ifname.toStdString().c_str(), net_id);
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "enableNetwork";
    callbackFunction = std::bind(&NMDaemonClient::saveConfig, this, ifname);
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    /*
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
    */
    return true;
}

int NMDaemonClient::getWpaNetworkId(QString ifname, QString ssid)
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
        json_object = getJsonForSsid(ssid);
        if(!json_object.empty())
        {
            net_id = json_object.value("network id").toString().toInt();
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
    int net_id = getWpaNetworkId(ifname, ssid);
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

bool NMDaemonClient::addNetwork(QString ifname, QString ssid, QString psk, bool hexkey)
{
    if(hexkey)
    {
        qDebug() << "Hex keys are not (yet) supported";
        return false;
    }
    QJsonValue json_value;
    QString req_string;
    QJsonObject req_object;
    QJsonObject json_object;
    QString caps;
    QString ies;
    QString security;
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    req_string = QString::asprintf(CMD_WIFI_SCAN_RESULTS.c_str(), ifname.toStdString().c_str());
    req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    if(!sendRequest(req_object))
    {
        return false;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        json_object = getJsonForSsid(ssid);
        if(!json_object.empty())
        {
            caps = json_object.value("CAPS").toString();
            ies = json_object.value("IES").toString();
            if( (!caps.isEmpty()) && (!ies.isEmpty()) )
            {
                security = getWifiNetworkSecurity(caps, ies);
            }
            if(security != WIFI_SECURITY_ERROR)
            {
                if(security == WIFI_SECURITY_OPEN)
                {
                    req_string = QString::asprintf(CMD_WPA_ADD_OPEN_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str());
                }
                else
                {
                    req_string = QString::asprintf(CMD_WPA_ADD_SECURE_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str(), psk.toStdString().c_str(), security.toStdString().c_str());
                }
                req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
                //readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
                //checkDataResultSource = "addNetwork";
                //callbackFunction = std::bind(&NMDaemonClient::saveConfig, this, ifname);
                //connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
                if(!sendRequest(req_object))
                {
                    return false;
                }
                /*
                {
                    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
                    if(!lastJsonReceived.empty())
                    {
                        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
                        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
                        {
                            return saveConfig(ifname);
                        }
                    }
                }
                */
            }
        }
    }
    return true;
}

// Used code from trueos-utils-qt5 QString NetworkInterface::parseWifiSecurity( QString caps, QString etc )
QString NMDaemonClient::getWifiNetworkSecurity(QString caps, QString ies)
{
    bool isDisabled = !caps.contains("P") && caps.contains("ES");
    bool isWEP = caps.contains("EP");
    bool isWPA = ies.contains("WPA");
    bool isWPA2 = ies.contains("RSN");
    QString securityType;

    // TODO: detect more security types (RADIUS etc.)
    if(isDisabled)
    {
      securityType = WIFI_SECURITY_OPEN;
    }
    else if(isWPA2)
    {
      securityType = WIFI_SECURITY_WPA2_PSK;
    }
    else if(isWPA)
    {
      securityType = WIFI_SECURITY_WPA_PSK;
    }
    else if(isWEP)
    {
      securityType = WIFI_SECURITY_WEP;
    }
    else
    {
      securityType = WIFI_SECURITY_ERROR;
    }
    return securityType;
}

bool NMDaemonClient::saveConfig(QString ifname)
{
    QJsonValue json_value;
    //std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QString req_string = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    checkDataResultSource = "saveConfig";
    callbackFunction = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

bool NMDaemonClient::checkSuccess(QJsonObject data)
{
    if(!data.empty())
    {
        QJsonValue json_value = data.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            return true;
        }
    }
    return false;
}

void NMDaemonClient::slotCheckDataResult(QJsonObject data)
{
    disconnect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    QString src = checkDataResultSource;
    checkDataResultSource = "";
    if(checkSuccess(data))
    {
        qDebug() << "SUCCESS received from " << src;
        if(callbackFunction != nullptr)
        {
            std::function<void()> cb = callbackFunction;
            callbackFunction = nullptr;
            cb();
        }
    }
    else
    {
        qDebug() << "ERROR received from " << src;
        emit dataError(QJsonDocument(data).toJson(QJsonDocument::Compact), src);
    }
}

