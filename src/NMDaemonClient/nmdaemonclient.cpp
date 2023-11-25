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
    connect(this, &NMDaemonClient::dataReceived, this, &NMDaemonClient::slotDataReceived);
}

NMDaemonClient::~NMDaemonClient()
{
    DoDisconnect();
    delete nmdUnixSocket;
}

bool NMDaemonClient::DoConnect()
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

bool NMDaemonClient::DoDisconnect()
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

QString NMDaemonClient::GetLastError()
{
    return lastError;
}

void NMDaemonClient::slotDataReceived()
{
    QJsonObject json_received;
    QString qstr_received = QString(buf);
    qDebug() << "slotDataReceived: Received from socket: " << qstr_received;
    QJsonDocument qjdoc_received = QJsonDocument::fromJson(qstr_received.toUtf8());
    if(!qjdoc_received.isNull())
    {
        if(qjdoc_received.isObject())
        {
            json_received = qjdoc_received.object();
        }
    }
    if(!isPersistentConn)
        DoDisconnect();
    if(writeSocket.available() == 0)
        writeSocket.release();
    if(!json_received.empty() && (json_received != lastJsonReceived) )
    {
        lastJsonReceived = json_received;
        emit dataReady(json_received);
    }
}
QJsonObject NMDaemonClient::AsyncSendRequest(QJsonObject json_command)
{
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
    auto json_received = QJsonObject();

    if(!DoConnect())
    {
        throw AsynStepException("AsyncSendRequest: DoConnect failed");
    }
    if(writeSocket.available()>0)
    {
        qDebug() << "AsyncSendRequest: writeSocket.acquire() direct for request " << QString::fromStdString(str_cmd);
        writeSocket.acquire();
    }
    else
    {
        bool available = false;
        int i=0,imax=200;
        long wt=50;
        for(i=0; i<imax; i++)
        {
            QCoreApplication::processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(wt));
            if(writeSocket.available()>0)
            {
                available=true;
                break;
            }
        }
        if(available)
        {
            qDebug() << "AsyncSendRequest: writeSocket.acquire() after waiting during " << (i-1)*wt << "msec for request " << QString::fromStdString(str_cmd);
            writeSocket.acquire();
        }
        else
        {
            qDebug() << "AsyncSendRequest: Cannot acquire write lock on nmdaemon socket for request " << QString::fromStdString(str_cmd);
            throw AsynStepException("AsyncSendRequest: Cannot acquire write lock on nmdaemon socket");
        }
    }

    lastJsonReceived = QJsonObject();
    memset(buf, 0, sizeof(buf));
    strlcpy(buf, str_cmd.c_str(), NM_MAXBUF);
    int len = (str_cmd.length()<NM_MAXBUF) ? str_cmd.length() : NM_MAXBUF-1;
    buf[len] = 0;
    qDebug() << "AsyncSendRequest: Writing to socket:\n\t" << buf;
    int len_sent = nmdUnixSocket->write(buf, sizeof(buf));
    if(len_sent<len+1)
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        qDebug() << "Error writing to socket: " << lastError;
        writeSocket.release();
        throw AsynStepException("AsyncSendRequest: Error writing to socket " + nmdUnixSocket->last_error_str());
    }

    memset(buf, 0, sizeof(buf));
    int n = nmdUnixSocket->read_n(buf, sizeof(buf));
    qDebug() << "AsyncSendRequest: Received from socket:\n\t" << buf;
    if(n<=0)
    {
        lastError = QString::fromStdString(nmdUnixSocket->last_error_str());
        qDebug() << "AsyncSendRequest: Error reading from socket: " << lastError;
        throw AsynStepException("AsyncSendRequest: Error reading from socket " + nmdUnixSocket->last_error_str());
    }

    QString qstr_received = QString(buf);
    //qDebug() << "AsyncSendRequest: Received from socket: " << qstr_received;
    QJsonDocument qjdoc_received = QJsonDocument::fromJson(qstr_received.toUtf8());
    if( !qjdoc_received.isNull() && qjdoc_received.isObject() )
        json_received = qjdoc_received.object();
    if(!isPersistentConn)
        DoDisconnect();
    if(writeSocket.available() == 0)
        writeSocket.release();
    if(!json_received.empty() && (json_received != lastJsonReceived) )
        lastJsonReceived = json_received;
    return json_received;
}

/*
bool NMDaemonClient::sendRequest(QJsonObject json_command)
{
    QString qstr_received = "";
    const char *ptr = nullptr;
    std::string str_cmd = QJsonDocument(json_command).toJson(QJsonDocument::Compact).toStdString();
    if(!DoConnect())
        return false;
    if(writeSocket.available()>0)
    {
        qDebug() << "writeSocket.acquire() direct for " << QString::fromStdString(str_cmd);
        writeSocket.acquire();
    }
    else
    {
        bool available = false;
        for(int i=0; i<100; i++)
        {
            QCoreApplication::processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(writeSocket.available()>0)
            {
                available=true;
                break;
            }
        }
        if(available)
        {
            qDebug() << "writeSocket.acquire() after waiting for " << QString::fromStdString(str_cmd);
            writeSocket.acquire();
        }
        else
        {
            qDebug() << "FATAL: Cannot acquire write lock on nmdaemon socket trying to write: " << QString::fromStdString(str_cmd);
            return false;
            //exit(1);
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
        //readData();    // TODO: call it in a separate thread ??

        QThread* read_thread = new QThread;
        auto read_sock = nmdUnixSocket->clone();
        //auto moved_sock = std::move(read_sock);
        NMDaemonWorker* worker = new NMDaemonWorker(std::move(read_sock), buf, NM_MAXBUF);
        worker->moveToThread(read_thread);
        connect(worker, SIGNAL(DataError(const QString&)), this, SLOT(SlotDataError(const QString&)), Qt::QueuedConnection);
        connect(worker, SIGNAL(DataRead()), this, SLOT(SlotReadData()), Qt::QueuedConnection);
        //connect(worker, SIGNAL(DataRead()), this, SLOT(slotDataReceived()), Qt::QueuedConnection);
        connect(read_thread, SIGNAL(started()), worker, SLOT(ReadData()), Qt::QueuedConnection);
        connect(worker, SIGNAL(Finished()), read_thread, SLOT(quit()), Qt::QueuedConnection);
        connect(worker, SIGNAL(Finished()), worker, SLOT(deleteLater()), Qt::QueuedConnection);
        connect(read_thread, SIGNAL(finished()), read_thread, SLOT(deleteLater()), Qt::QueuedConnection);
        read_thread->start();
        return true;
    }
}
*/

void NMDaemonClient::SlotDataError(const QString& error)
{
    lastError = error;
    qDebug() << "ERROR: Cannot read data from socket: " << error;
}

void NMDaemonClient::SlotReadData()
{
    //qDebug() << "SlotReadData";
    emit dataReceived();
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

bool NMDaemonClient::ConnectWifiNetwork(QString ifname, int netid)
{
    QString req_string1;
    QJsonObject req_object1;
    req_string1 = QString::asprintf(CMD_CONNECT_WIFI_NET_ID.c_str(), ifname.toStdString().c_str(), netid);
    req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
    QString source1 = "ConnectWifiNetwork";
    lastJsonReceived = QJsonObject();
    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object1); },
            [&](QJsonObject jobj){ return CheckConnectSuccess(jobj, "", netid); },
            source1.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
            );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    try
    {
        wf1->Run();
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError("ConnectWifiNetwork", "Error:\n" + QString(we.what()));
        return false;
    }
}

bool NMDaemonClient::ConnectWifiNetwork(QString ifname, QString ssid)
{
    QString req_string1;
    QJsonObject req_object1;
    req_string1 = QString::asprintf(CMD_CONNECT_WIFI_NET_SSID.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str());
    req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
    QString source1 = "ConnectWifiNetwork";
    lastJsonReceived = QJsonObject();
    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object1); },
            [&](QJsonObject jobj){ return CheckConnectSuccess(jobj, ssid, -1); },
            source1.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    try
    {
        wf1->Run();
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError("ConnectWifiNetwork", "Error:\n" + QString(we.what()));
        return false;
    }
}

bool NMDaemonClient::CheckConnectSuccess(QJsonObject result, QString ssid, int netid)
{
    if(CheckSuccess(result))
    {
        if(!ssid.isEmpty())
            emit successWifiConnect(ssid);
        else if(netid>=0)
            emit successWifiConnect(netid);
        return true;
    }
    // TODO: Check currently connected network to see if the connect request was finally succeeded
    // req_string = QString::asprintf(CMD_WIFI_STATUS.c_str(), s.toStdString().c_str());
    // .....
    auto json_value = result.value(QString::fromStdString(JSON_PARAM_RESULT));
    if(!ssid.isEmpty())
        emit errorWifiConnect(ssid, json_value.toString());
    else if(netid>=0)
        emit errorWifiConnect(netid, json_value.toString());
    return false;
}

QJsonObject NMDaemonClient::GetJsonForSsid(QString ssid)
{
    return GetJsonForSsid(ssid, lastJsonReceived);
}

QJsonObject NMDaemonClient::GetJsonForSsid(QString ssid, QJsonObject list_networks)
{
    QJsonArray json_array;
    QJsonValue json_value;
    QJsonObject json_object;
    QJsonObject ret_object;
    QString conf_ssid;

    json_value = list_networks.value(QString::fromStdString(JSON_PARAM_RESULT));
    if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
    {
        json_value = list_networks.value(QString::fromStdString(JSON_PARAM_DATA));
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

int NMDaemonClient::GetWpaNetworkId(QString ifname, QString ssid)
// Get list of configured networks from WPA and search for the ssid in this list
{
    int net_id = -1;
    auto list_networks = AsyncGetWpaNetworkList(ifname, false);
    auto json_object = GetJsonForSsid(ssid, list_networks);
    if(!json_object.empty())
    {
        net_id = json_object.value("network id").toString().toInt();
    }
    return net_id;
}

bool NMDaemonClient::CheckUpdateWpaNetworkListResult(QJsonObject wpa_list)
{
    QSharedPointer<QVector<QJsonObject>> wpa_networks = QSharedPointer<QVector<QJsonObject>>(new QVector<QJsonObject>());
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;

    if(!CheckSuccess(wpa_list))
        return false;
    if(!wpa_list.empty())
    {
        json_value = wpa_list.value(QString::fromStdString(JSON_PARAM_DATA));
        json_object = json_value.toObject();
        json_value = json_object.value(QString::fromStdString(JSON_PARAM_NETWORKS));
        json_array = json_value.toArray();
        foreach (const QJsonValue &v, json_array)
        {
            json_object = v.toObject();
            if(!json_object.isEmpty())
                wpa_networks->push_back(json_object);
        }
    }
    emit UpdatedWpaNetworkList(wpa_networks);
    return true;
}

/*
void NMDaemonClient::CBUpdateWpaNetworkList(QString)
{
    std::shared_ptr<QVector<QJsonObject>> wpa_networks = std::make_shared<QVector<QJsonObject>>();
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;

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
                    wpa_networks->push_back(json_object);
            }
        }
    }
    emit UpdatedWpaNetworkList(wpa_networks);
}
*/

bool NMDaemonClient::UpdateWpaNetworkList(QString ifname, bool detailed)
{
    QString req_string1;
    if(detailed)
        req_string1 = QString::asprintf(CMD_WPA_LIST_NETWORKS_DETAILED.c_str(), ifname.toStdString().c_str());
    else
        req_string1 = QString::asprintf(CMD_WPA_LIST_NETWORKS.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
    QString source1 = "UpdateWpaNetworkList";

    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object1); },
            [this](QJsonObject jobj){ return CheckUpdateWpaNetworkListResult(jobj); },
            source1.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    try
    {
        wf1->Run();
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError("UpdateWpaNetworkList", "Error sending request:\n" + QString(we.what()));
        return false;
    }
}

bool NMDaemonClient::UpdatePskAndSave(QString ifname, int net_id, QString psk, bool hexkey)
{
    if(hexkey)
    {
        qDebug() << "Hex keys are not (yet) supported";
        return false;
    }

    QString req_string1 = QString::asprintf(CMD_WPA_UPDATE_PSK.c_str(), ifname.toStdString().c_str(), net_id, psk.toStdString().c_str());
    QJsonObject req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
    QString source1 = "UpdatePsk";
    QString req_string2 = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object2 = QJsonDocument::fromJson(req_string2.toUtf8()).object();
    QString source2 = "SaveConfig";

    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object1); },
            [this](QJsonObject jobj){ return CheckSuccess(jobj); },
            source1.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    auto step2 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object2); },
            [this](QJsonObject jobj){ return CheckSuccess(jobj); },
            source2.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    wf1->AddStep(step2);
    try
    {
        wf1->Run();
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError("UpdatePskAndSave", "Error sending request:\n" + QString(we.what()));
        return false;
    }
}

bool NMDaemonClient::UpdatePskAndSave(QString ifname, QString ssid, QString psk, bool hexkey)
{
    int net_id = -1;
    auto list_networks = AsyncGetWpaNetworkList(ifname, false);
    auto json_object = GetJsonForSsid(ssid, list_networks);
    if(json_object.empty())
    {
        qDebug() << "UpdatePskAndSave: Cannot get net_id for ssid " << ssid << " from json received:\n\t" << QJsonDocument(list_networks).toJson(QJsonDocument::Compact);
        return false;
    }
    net_id = json_object.value("network id").toString().toInt();
    if(net_id<0)
    {
        qDebug() << "UpdatePskAndSave: Got invalid net_id " << net_id << " for ssid " << ssid << " from json received:\n\t" << QJsonDocument(list_networks).toJson(QJsonDocument::Compact);
        return false;
    }
    return UpdatePskAndSave(ifname, net_id, psk, hexkey);
}

QJsonObject NMDaemonClient::AsyncGetWpaNetworkList(QString ifname, bool detailed)
{
    QString req_string1;
    if(detailed)
        req_string1 = QString::asprintf(CMD_WPA_LIST_NETWORKS_DETAILED.c_str(), ifname.toStdString().c_str());
    else
        req_string1 = QString::asprintf(CMD_WPA_LIST_NETWORKS.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommandGetResult(req_string1, "AsyncGetWpaNetworkList");
}

bool NMDaemonClient::AddNetwork(QString ifname, QString ssid, QString psk, bool hexkey)
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
    QJsonObject json_object_scan;
    QString caps;
    QString ies;
    QString security;
    std::unique_ptr<NMDaemonSpy> readspy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    req_string = QString::asprintf(CMD_WIFI_SCAN_RESULTS.c_str(), ifname.toStdString().c_str());
    json_object_scan = AsyncSendCommandGetResult(req_string, "AddNetwork");

    if(!json_object_scan.empty())
    {
        json_object = GetJsonForSsid(ssid, json_object_scan);
        if(!json_object.empty())
        {
            caps = json_object.value("CAPS").toString();
            ies = json_object.value("IES").toString();
            if( (!caps.isEmpty()) && (!ies.isEmpty()) )
            {
                security = GetWifiNetworkSecurity(caps, ies);
            }
            return AddNetwork(ifname, ssid, psk, security, hexkey);
        }
        else
        {
            qDebug() << "Cannot get last scan results for SSID " << ssid;
            return false;
        }
    }
    else
    {
        qDebug() << "Cannot get last scan results";
        return false;
    }
}

bool NMDaemonClient::AddNetwork(QString ifname, QString ssid, QString psk, QString security, bool hexkey)
{
    QString req_string1;
    QJsonObject req_object;

    if(hexkey)
    {
        qDebug() << "Hex keys are not (yet) supported";
        return false;
    }

    if(security != WIFI_SECURITY_ERROR)
    {
        if(security == WIFI_SECURITY_OPEN)
        {
            req_string1 = QString::asprintf(CMD_WPA_ADD_OPEN_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str());
        }
        else
        {
            req_string1 = QString::asprintf(CMD_WPA_ADD_SECURE_NETWORK.c_str(), ifname.toStdString().c_str(), ssid.toStdString().c_str(), psk.toStdString().c_str(), security.toStdString().c_str());
        }
        QJsonObject req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
        QString source1 = "AddNetwork";
        QString req_string2 = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
        QJsonObject req_object2 = QJsonDocument::fromJson(req_string2.toUtf8()).object();
        QString source2 = "SaveConfig";

        auto step1 = std::make_shared<AsynStep<QJsonObject>>
            (
                [&](){ return AsyncSendRequest(req_object1); },
                [&](QJsonObject jobj){ return CheckAddNetworkResult(jobj, ssid); },
                source1.toStdString(),
                [](){ QCoreApplication::processEvents(); },
                200
            );
        auto step2 = std::make_shared<AsynStep<QJsonObject>>
            (
                [&](){ return AsyncSendRequest(req_object2); },
                [this](QJsonObject jobj){ return CheckSuccess(jobj); },
                source2.toStdString(),
                [](){ QCoreApplication::processEvents(); },
                200
            );
        std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
        wf1->AddStep(step1);
        wf1->AddStep(step2);
        try
        {
            wf1->Run();
            return true;
        }
        catch (AsynWfException we)
        {
            emit dataError("AddNetwork", "Error sending request:\n" + QString(we.what()));
            return false;
        }
    }
    else
    {
        qDebug() << "Unknown WiFi security";
        return false;
    }
}

bool NMDaemonClient::CheckAddNetworkResult(QJsonObject add_network_result, QString ssid)
{
    QJsonValue json_value;
    int netid = -1;

    if(!CheckSuccess(add_network_result))
    {
        emit errorWifiNetAdd(ssid, QString(QJsonDocument(add_network_result).toJson(QJsonDocument::Compact)));
        return false;
    }

    json_value = add_network_result.value(QString::fromStdString(JSON_PARAM_DATA));
    json_value = json_value.toObject().value("NETWORK");
    netid = json_value.toObject().value("network id").toString().toInt();
    if(netid >= 0)
    {
        emit successWifiNetAdd(ssid, netid);
        return true;
    }
    else
    {
        emit errorWifiNetAdd(ssid, QString(QJsonDocument(add_network_result).toJson(QJsonDocument::Compact)));
        return false;
    }
}

/*
void NMDaemonClient::cbWifiNetAdd(QString ifname, QString ssid)
{
    QJsonValue json_value;
    int netid = -1;
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_RESULT));
        if(json_value.toString().toStdString() == JSON_PARAM_SUCC)
        {
            json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
            json_value = json_value.toObject().value("NETWORK");
            netid = json_value.toObject().value("network id").toString().toInt();
            lastJsonReceived = QJsonObject();
            checkDataResultSource = "cbWifiNetAdd";
            callbackFunctionSuccess = std::bind(&NMDaemonClient::cbSendSuccessWifiNetAdd, this, ssid, netid);
            SaveConfig(ifname);
        }
        else
        {
            emit errorWifiNetAdd(ssid, json_value.toString());
        }
    }
    else
    {
        emit errorWifiNetAdd(ssid, QString::fromStdString(JSON_PARAM_ERR));
    }
}
*/

// Used code from trueos-utils-qt5 QString NetworkInterface::parseWifiSecurity( QString caps, QString etc )
QString NMDaemonClient::GetWifiNetworkSecurity(QString caps, QString ies) const
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

bool NMDaemonClient::SaveConfig(QString ifname)
{
    QString req_string = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "SaveConfig");
}

bool NMDaemonClient::CheckSuccess(QJsonObject data)
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
    if(CheckSuccess(data))
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

QString NMDaemonClient::GetIPv4Address(QString ifname)
{
    QJsonObject json_object;
    QJsonValue json_value;
    QString addr = "";
    QString req_string = QString::asprintf(CMD_GET_IPV4_ADDRESS.c_str(), ifname.toStdString().c_str());
    QJsonObject ret_object = AsyncSendCommandGetResult(req_string, "GetIPv4Address");
    if(ret_object.empty())
        return addr;

    json_value = ret_object.value(QString::fromStdString(JSON_PARAM_DATA));
    json_object = json_value.toObject();
    if(!json_object.empty())
    {
        json_value = json_object.value(QString::fromStdString(JSON_PARAM_IPV4_ADDR));
        addr = json_value.toString();
    }
    return addr;
}

bool NMDaemonClient::IsDhcp4Enabled(QString ifname)
{
    QJsonObject json_object;
    QJsonValue json_value;
    QString str_status;
    bool status = false;
    QString req_string = QString::asprintf(CMD_GET_IPV4_DHCP_STATUS.c_str(), ifname.toStdString().c_str());
    QJsonObject ret_object = AsyncSendCommandGetResult(req_string, "IsDhcp4Enabled");
    if(ret_object.empty())
        return status;

    json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
    json_object = json_value.toObject();
    if(!json_object.empty())
    {
        json_value = json_object.value(QString::fromStdString(JSON_DATA_DHCP_STATUS));
        str_status = json_value.toString();
        if(str_status.toStdString()==JSON_DATA_ENABLED)
            status = true;
    }
    return status;
}

QJsonObject NMDaemonClient::AsyncSendCommandGetResult(QString command, QString source)
{
    QJsonObject ret_object = QJsonObject();
    QJsonObject req_object = QJsonDocument::fromJson(command.toUtf8()).object();
    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object); },
            [this](QJsonObject jobj){ return CheckSuccess(jobj); },
            source.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    try
    {
        if(!step1->Run())
        {
            qDebug() << "AsyncSendCommandGetResult: Command failed: " << command;
            return ret_object;
        }
    }
    catch (AsynStepException se)
    {
        qDebug() << "AsyncSendCommandGetResult: Command " << command << " failed: " << QString(se.what());
        return ret_object;
    }
    ret_object = step1->GetData();
    return ret_object;
}

bool NMDaemonClient::AsyncSendCommand(QString command, QString source)
{
    QJsonObject req_object = QJsonDocument::fromJson(command.toUtf8()).object();
    auto step1 = std::make_shared<AsynStep<QJsonObject>>
    (
        [&](){ return AsyncSendRequest(req_object); },
        [this](QJsonObject jobj){ return CheckSuccess(jobj); },
        source.toStdString(),
        [](){ QCoreApplication::processEvents(); },
        200
    );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    try
    {
        wf1->Run();
        emit dataSuccess(QJsonDocument(req_object).toJson(QJsonDocument::Compact), source);
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError(source, "Error sending request:\n" + QString(we.what()) + "\nRequest was:\n" + QJsonDocument(req_object).toJson(QJsonDocument::Compact));
        return false;
    }
}

/*
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
*/

bool NMDaemonClient::SetIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return AsyncSendCommand(req_string, "SetIPv4Address");
}

bool NMDaemonClient::AddIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_ADD_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return AsyncSendCommand(req_string, "AddIPv4Address");
}

bool NMDaemonClient::RemoveIPv4Address(QString ifname, QString address, QString mask)
{
    QString req_string = QString::asprintf(CMD_REMOVE_IPV4_ADDRESS.c_str(),
                                                ifname.toStdString().c_str(),
                                                address.toStdString().c_str(),
                                                mask.toStdString().c_str() );
    return AsyncSendCommand(req_string, "RemoveIPv4Address");
}

bool NMDaemonClient::EnableInterface(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IF_ENABLE.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "EnableInterface");
}

bool NMDaemonClient::DisableInterface(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IF_DISABLE.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "DisableInterface");
}

bool NMDaemonClient::EnableDhcp4(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_DHCP_ENABLE.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "EnableDhcp4");
}

bool NMDaemonClient::DisableDhcp4(QString ifname)
{
    QString req_string = QString::asprintf(CMD_SET_IPV4_DHCP_DISABLE.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "DisableDhcp4");
}

bool NMDaemonClient::ResetWpa(QString ifname)
{
    QString req_string = QString::asprintf(CMD_WPA_RESET.c_str(), ifname.toStdString().c_str());
    return AsyncSendCommand(req_string, "ResetWpa");
}

bool NMDaemonClient::SetNetworkEnabled(QString ifname, int netid, bool is_enabled)
{
    QString req_string;
    if(is_enabled)
        req_string = QString::asprintf(CMD_WPA_ENABLE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);
    else
        req_string = QString::asprintf(CMD_WPA_DISABLE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);
    return AsyncSendCommand(req_string, "SetNetworkEnabled");
}

bool NMDaemonClient::SetNetworkPriority(QString ifname, int netid, int priority)
{
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_PRI.c_str(), ifname.toStdString().c_str(), netid, priority);
    return AsyncSendCommand(req_string, "SetNetworkPriority");
}

bool NMDaemonClient::SetNetworkSsid(QString ifname, int netid, QString ssid)
{
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_SSID.c_str(), ifname.toStdString().c_str(), netid, ssid.toStdString().c_str());
    return AsyncSendCommand(req_string, "SetNetworkSsid");
}

bool NMDaemonClient::SetNetworkBssid(QString ifname, int netid, QString bssid)
{
    if(bssid=="any")
        bssid="00:00:00:00:00:00";
    QString req_string = QString::asprintf(CMD_WPA_SET_NETWORK_BSSID.c_str(), ifname.toStdString().c_str(), netid, bssid.toStdString().c_str());
    return AsyncSendCommand(req_string, "SetNetworkBssid");
}

bool NMDaemonClient::RemoveNetwork(QString ifname, int netid)
{
    QString req_string = QString::asprintf(CMD_WPA_REMOVE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);
    return AsyncSendCommand(req_string, "RemoveNetwork");
}

bool NMDaemonClient::RemoveNetworkAndSave(QString ifname, int netid)
{
    QString req_string1 = QString::asprintf(CMD_WPA_REMOVE_NETWORK.c_str(), ifname.toStdString().c_str(), netid);
    QJsonObject req_object1 = QJsonDocument::fromJson(req_string1.toUtf8()).object();
    QString source1 = "RemoveNetwork";
    QString req_string2 = QString::asprintf(CMD_WPA_SAVE.c_str(), ifname.toStdString().c_str());
    QJsonObject req_object2 = QJsonDocument::fromJson(req_string2.toUtf8()).object();
    QString source2 = "SaveConfig";

    auto step1 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object1); },
            [this](QJsonObject jobj){ return CheckSuccess(jobj); },
            source1.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    auto step2 = std::make_shared<AsynStep<QJsonObject>>
        (
            [&](){ return AsyncSendRequest(req_object2); },
            [this](QJsonObject jobj){ return CheckSuccess(jobj); },
            source2.toStdString(),
            [](){ QCoreApplication::processEvents(); },
            200
        );
    std::unique_ptr<AsynWf> wf1 = std::make_unique<AsynWf>();
    wf1->AddStep(step1);
    wf1->AddStep(step2);
    try
    {
        wf1->Run();
        return true;
    }
    catch (AsynWfException we)
    {
        emit dataError("RemoveNetworkAndSave", "Error sending request:\n" + QString(we.what()));
        return false;
    }
}

QPair<QString, QString> NMDaemonClient::GetRouteDefault4()
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    QPair<QString, QString> def_route;  // ifname, ip4_address
    QString def_route_addr4 = "";
    QString ifname = "";
    QString req_string = QString::fromStdString(CMD_ROUTE_GET_DEFAULT);
    QJsonObject ret_object = AsyncSendCommandGetResult(req_string, "GetRouteDefault");
    if(ret_object.empty())
    {
        return QPair<QString, QString>("", "");
    }
    json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
    json_object = json_value.toObject();
    json_value = json_object.value(QString::fromStdString(JSON_PARAM_IPV4_GW));
    def_route_addr4 = json_value.toString();
    if(def_route_addr4.isEmpty())
    {
        return QPair<QString, QString>("", "");
    }

    req_string = QString::fromStdString(CMD_ROUTES_LIST);
    ret_object = AsyncSendCommandGetResult(req_string, "GetRouteDefault");
    if(ret_object.empty())
    {
        return QPair<QString, QString>("", "");
    }
    json_value = lastJsonReceived.value(QString::fromStdString(JSON_PARAM_DATA));
    json_object = json_value.toObject();
    json_value = json_object.value(QString::fromStdString(JSON_PARAM_ROUTES));
    json_array = json_value.toArray();
    foreach (const QJsonValue &v, json_array)   // Loop routes array
    {
        if(!v.toObject().value(QString::fromStdString(JSON_PARAM_UP)).toBool())
            continue;
        ifname = v.toObject().value(QString::fromStdString(JSON_PARAM_IF_NAME)).toString();
        if(ifname.isEmpty())
            continue;
        QJsonArray json_array1 = v.toObject().value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
        foreach (const QJsonValue &v1, json_array1) // Loop addresses array
        {
            if(v1.toObject().value(QString::fromStdString(JSON_PARAM_ADDR_TYPE))=="ROUTE")
            {
                QJsonObject json_route = v1.toObject().value("ROUTE").toObject();
                if(json_route.value(QString::fromStdString(JSON_PARAM_IPV4_ADDR))!="0.0.0.0")
                    continue;
                if(json_route.value(QString::fromStdString(JSON_PARAM_IPV4_GW))==def_route_addr4)
                {
                    QJsonArray json_array2 = json_route.value(QString::fromStdString(JSON_PARAM_FLAGS)).toArray();
                    foreach (const QJsonValue &v2, json_array2)
                    {
                        if(v2.toString()==QString::fromStdString(JSON_PARAM_UP))
                            return QPair<QString, QString>(ifname, def_route_addr4);
                    }
                }
            }
        }
    }
    return QPair<QString, QString>("", "");
}

bool NMDaemonClient::SetDefaultGateway4(QString ip4_gateway)
{
    QString req_string = QString::asprintf(CMD_ROUTE_SET_DEFAULT.c_str(), ip4_gateway.toStdString().c_str());
    return AsyncSendCommand(req_string, "SetDefaultGateway4");
}
