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
    callbackFunctionSuccess = nullptr;
    callbackFunctionError = nullptr;
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

bool NMDaemonClient::sendRequest(QJsonObject json_command)
{
    QString qstr_received = "";
    const char *ptr = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
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

bool NMDaemonClient::connectWifiNetwork(QString ifname, QString ssid)
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    QString conf_ssid;
    QString req_string;
    QJsonObject req_object;
    QString flags;
    req_string = QString::asprintf(CMD_CONNECT_WIFI_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str());
    req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "connectWifiNetwork";
    callbackFunctionSuccess = std::bind(&NMDaemonClient::cbWifiConnect, this, ifname, ssid);
    callbackFunctionError = callbackFunctionSuccess;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

void NMDaemonClient::cbWifiConnect(QString, QString ssid)
{
    QJsonValue json_value;
    if(!lastJsonReceived.empty())
//{ { JSON_PARAM_RESULT, JSON_DATA_ERR_AUTH_FAILED } }
//{ { JSON_PARAM_RESULT, JSON_DATA_ERR_NOT_AVAILABLE } }
//...
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
            emit successWifiConnect(ssid);
        else
            // TODO: Check currently connected network to see if the connect request was finally succeeded
            emit errorWifiConnect(ssid, json_value.toString());
    }
    else
    {
        emit errorWifiConnect(ssid, QString::fromStdString(JSON_PARAM_ERR));
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

bool NMDaemonClient::enableNetwork(QString ifname, QString ssid) // TODO: Rename to enableWifiNetwork
{
    int net_id = getWpaNetworkId(ifname, ssid);
    if(net_id<0)
        return false;
    QJsonValue json_value;
    QString req_string = QString::asprintf(CMD_WPA_ENABLE_NETWORK.c_str(), ifname.toStdString().c_str(), net_id);
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "enableNetwork";
    callbackFunctionSuccess = std::bind(&NMDaemonClient::cbWifiNetEnable, this, ifname, ssid);
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

void NMDaemonClient::cbWifiNetEnable(QString ifname, QString ssid)
{
    QJsonValue json_value;
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            lastJsonReceived = QJsonObject();
            checkDataResultSource = "cbWifiNetEnable";
            callbackFunctionSuccess = std::bind(&NMDaemonClient::cbSendSuccessWifiNetEnable, this, ssid);
            saveConfig(ifname);
        }
        else
        {
            // TODO: Check currently connected network to see if the connect request was finally succeeded
            emit errorWifiNetEnable(ssid, json_value.toString());
        }
    }
    else
    {
        emit errorWifiNetEnable(ssid, QString::fromStdString(JSON_PARAM_ERR));
    }
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

QVector<QJsonObject> NMDaemonClient::getWpaNetworkList(QString ifname)
{
    QVector<QJsonObject> ret_nets;
    QString req_string = QString::asprintf(CMD_WPA_LIST_NETWORKS.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;

    if(!sendRequest(req_object))
    {
        return ret_nets;
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
                if(!json_object.isEmpty())
                    ret_nets.push_back(json_object);
            }
        }
    }
    return ret_nets;
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
    QString req_string = QString::asprintf(CMD_WPA_UPDATE_PSK.c_str(), ifname.toStdString().c_str(), net_id, psk.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "updatePsk";
    callbackFunctionSuccess = std::bind(&NMDaemonClient::saveConfig, this, ifname);
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
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
                checkDataResultSource = "addNetwork";
                callbackFunctionSuccess = std::bind(&NMDaemonClient::cbWifiNetAdd, this, ifname, ssid);
                connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
                if(!sendRequest(req_object))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void NMDaemonClient::cbWifiNetAdd(QString ifname, QString ssid)
{
    QJsonValue json_value;
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            lastJsonReceived = QJsonObject();
            checkDataResultSource = "cbWifiNetAdd";
            callbackFunctionSuccess = std::bind(&NMDaemonClient::cbSendSuccessWifiNetAdd, this, ssid);
            saveConfig(ifname);
        }
        else
        {
            // TODO: Check currently connected network to see if the connect request was finally succeeded
            emit errorWifiNetAdd(ssid, json_value.toString());
        }
    }
    else
    {
        emit errorWifiNetAdd(ssid, QString::fromStdString(JSON_PARAM_ERR));
    }
}

// Used code from trueos-utils-qt5 QString NetworkInterface::parseWifiSecurity( QString caps, QString etc )
QString NMDaemonClient::getWifiNetworkSecurity(QString caps, QString ies) const
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
    QString req_string = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    return sendCommand(req_string, "saveConfig");
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
        if(callbackFunctionSuccess != nullptr)
        {
            std::function<void()> cb = callbackFunctionSuccess;
            callbackFunctionSuccess = nullptr;
            cb();
        }
        else
        {
            emit dataSuccess(QJsonDocument(data).toJson(QJsonDocument::Compact), src);
        }
    }
    else
    {
        qDebug() << "ERROR received from " << src;
        if(callbackFunctionError != nullptr)
        {
            std::function<void()> cb = callbackFunctionError;
            callbackFunctionError = nullptr;
            cb();
        }
        else
        {
            emit dataError(QJsonDocument(data).toJson(QJsonDocument::Compact), src);
        }
    }
}

QString NMDaemonClient::getIPv4Address(QString ifname)
{
    QString req_string = QString::asprintf(CMD_GET_IPV4_ADDRESS.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QJsonObject json_object;
    QJsonValue json_value;
    QString addr;

    if(!sendRequest(req_object))
    {
        return addr;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        if(checkSuccess(lastJsonReceived))
        {
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_object = json_value.toObject();
            if(!json_object.empty())
            {
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_IPV4_ADDR));
                addr = json_value.toString();
            }
        }
    }
    return addr;
}

bool NMDaemonClient::isDhcp4Enabled(QString ifname)
{
    QString req_string = QString::asprintf(CMD_GET_IPV4_DHCP_STATUS.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    QJsonObject json_object;
    QJsonValue json_value;
    QString str_status;
    bool status = false;

    if(!sendRequest(req_object))
    {
        return status;
    }
    readspy->wait(DEFAULT_READ_WAIT_TIME_MSEC);
    if(!lastJsonReceived.empty())
    {
        if(checkSuccess(lastJsonReceived))
        {
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_object = json_value.toObject();
            if(!json_object.empty())
            {
                json_value = json_object.value(QString::fromStdString(JSON_DATA_DHCP_STATUS));
                str_status = json_value.toString();
                if(str_status.toStdString()==JSON_DATA_ENABLED)
                    status = true;
            }
        }
    }
    return status;
}

bool NMDaemonClient::sendCommand(QString command, QString source)
{
    QJsonObject req_object = QJsonDocument::fromJson(command.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = source;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

bool NMDaemonClient::setIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return sendCommand(req_string, "setIPv4Address");
}

bool NMDaemonClient::addIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_ADD_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return sendCommand(req_string, "addIPv4Address");
}

bool NMDaemonClient::removeIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_REMOVE_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return sendCommand(req_string, "removeIPv4Address");
}

bool NMDaemonClient::enableInterface(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IF_ENABLE.c_str(), ifname.toStdString().c_str());
    return sendCommand(req_string, "enableInterface");
}

bool NMDaemonClient::disableInterface(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IF_DISABLE.c_str(), ifname.toStdString().c_str());
    return sendCommand(req_string, "disableInterface");
}

bool NMDaemonClient::enableDhcp4(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_DHCP_ENABLE.c_str(), ifname.toStdString().c_str());
    return sendCommand(req_string, "enableDhcp4");
}

bool NMDaemonClient::disableDhcp4(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_DHCP_DISABLE.c_str(), ifname.toStdString().c_str());
    return sendCommand(req_string, "disableDhcp4");
}

bool NMDaemonClient::SetNetworkEnabled(QString ifname, int netid, bool is_enabled)
{
    QString req_string;
    if(is_enabled)
        req_string = QString::asprintf(CMD_WPA_ENABLE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);
    else
        req_string = QString::asprintf(CMD_WPA_DISABLE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);

    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "SetNetworkEnabled";
    callbackFunctionSuccess = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

bool NMDaemonClient::SetNetworkPriority(QString ifname, int netid, int priority)
{
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_PRI.c_str(), ifname.toStdString().c_str(), netid, priority);

    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "SetNetworkPriority";
    callbackFunctionSuccess = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

bool NMDaemonClient::SetNetworkSsid(QString ifname, int netid, QString ssid)
{
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_SSID.c_str(), ifname.toStdString().c_str(), netid, ssid.toStdString().c_str());

    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "SetNetworkSsid";
    callbackFunctionSuccess = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}

bool NMDaemonClient::SetNetworkBssid(QString ifname, int netid, QString bssid)
{
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_BSSID.c_str(), ifname.toStdString().c_str(), netid, bssid.toStdString().c_str());
/* NOT IMPLEMENTED YET
    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "SetNetworkBssid";
    callbackFunctionSuccess = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
*/
    return true;
}

bool NMDaemonClient::RemoveNetwork(QString ifname, int netid)
{
    QString req_string = QString::asprintf(CMD_WPA_REMOVE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);

    QJsonObject req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    checkDataResultSource = "RemoveNetwork";
    callbackFunctionSuccess = nullptr;
    connect(this, &NMDaemonClient::dataReady, this, &NMDaemonClient::slotCheckDataResult);
    if(!sendRequest(req_object))
    {
        return false;
    }
    return true;
}
