#include "nmdaemoncheckstatus.h"

NMDaemonCheckStatus::NMDaemonCheckStatus(int refresh_time, QString socket_path) : NMDaemonClient(socket_path, true),
                                                              refreshTime(refresh_time), isMonitoring(false)
{
    spy = nullptr;
    timerMonitoring = new QTimer(this);
    timerMonitoring->setInterval(refreshTime);
    connect(timerMonitoring, &QTimer::timeout, this, &NMDaemonCheckStatus::updateStatus);
//    connect(nmdSocket, &QLocalSocket::disconnected, timerMonitoring, &QTimer::stop);
}

NMDaemonCheckStatus::NMDaemonCheckStatus(QString socket_path) : NMDaemonCheckStatus(REFRESH_TIME_DEFAULT_MSEC, socket_path)
{
}

NMDaemonCheckStatus::NMDaemonCheckStatus(int refresh_time) : NMDaemonCheckStatus(refresh_time, NMD_SOCKET_DEFAULT)
{
}

NMDaemonCheckStatus::NMDaemonCheckStatus() : NMDaemonCheckStatus(REFRESH_TIME_DEFAULT_MSEC)
{
}

NMDaemonCheckStatus::~NMDaemonCheckStatus()
{
    if(isMonitoring)
        stopMonitoring();
    delete timerMonitoring;
}

void NMDaemonCheckStatus::startMonitoring()
{
    if(isMonitoring)
        return;
    if(!doConnect())
        return;
    isMonitoring = true;
    updateStatus();
    timerMonitoring->start();
}

void NMDaemonCheckStatus::slotStartMonitoring()
{
    startMonitoring();
}

void NMDaemonCheckStatus::stopMonitoring()
{
    if(isMonitoring)
        timerMonitoring->stop();
    isMonitoring = false;
}

void NMDaemonCheckStatus::updateStatus()
{
    std::unique_ptr<NMDaemonSpy> spy = nullptr;
    timerMonitoring->stop();
    if(!doConnect())
        return;
    QString if_name;
    QString req_string;
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    bool iflistupdated = false;
    bool netstatusupdated = false;
    bool ifstatusupdated = false;
    // Get list of all netwok interfaces
    lastIfStatus = QJsonArray();
    QJsonObject req_object = QJsonDocument::fromJson(CMD_GET_SYS_IF_STATUS.toUtf8()).object();
    //spy = new NMDaemonSpy(this, &NMDaemonCheckStatus::dataReady);
    spy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    if(!sendRequest(req_object))
        goto label_exit;
    if(!spy->wait(DEFAULT_READ_WAIT_TIME_MSEC))
        writeSocket.release();
    if(!lastJsonReceived.empty())
    {
        json_value = lastJsonReceived.value("RESULT");
        if(json_value.toString() == "SUCCESS")
        {
            json_value = lastJsonReceived.value("DATA");
            json_array = json_value.toArray();
            lastIfStatus = json_array;
            //emit ifStatusUpdated();
            ifstatusupdated = true;
        }
    }
    // Get list of wireless interfaces available for wpa_supplicant
    req_object = QJsonDocument::fromJson(CMD_GET_WPA_IF_LIST.toUtf8()).object();
    lastJsonReceived = QJsonObject();
    wifiInterfaces.clear();
    //spy = new NMDaemonSpy(this, &NMDaemonCheckStatus::dataReady);
    spy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
    if(!sendRequest(req_object))
        goto label_exit;
    if(!spy->wait(DEFAULT_READ_WAIT_TIME_MSEC))
        writeSocket.release();
    if(!lastJsonReceived.empty())
//    { "DATA": { "INTERFACES": [ "wlan0", "wlan1" ] }, "RESULT": "SUCCESS" }
    {
        json_value = lastJsonReceived.value("RESULT");
        if(json_value.toString() == "SUCCESS")
        {
            json_value = lastJsonReceived.value("DATA");
            json_object = json_value.toObject();
            json_value = json_object.value("INTERFACES");
            json_array = json_value.toArray();
            foreach (const QJsonValue &v, json_array)
            {
                if_name = v.toString();
                wifiInterfaces.push_back(if_name);
            }
        }
    }
    //emit wifiIfListUpdated();
    iflistupdated = true;
    lastWifiNetStatus.clear();
    // Get list of wireless networks for each wireless interface
    foreach(const QString &s, wifiInterfaces)
    {
        req_string = QString::asprintf(CMD_WIFI_SCAN.c_str(), s.toStdString().c_str());
        req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
        //spy = new NMDaemonSpy(this, &NMDaemonCheckStatus::dataReady);
        spy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
        if(!sendRequest(req_object))
            goto label_exit;
        if(!spy->wait(SCAN_READ_WAIT_TIME_MSEC))
            writeSocket.release();
        if(!lastJsonReceived.empty())
        {
            json_value = lastJsonReceived.value("RESULT");
            if(json_value.toString() == "SUCCESS")
            {
                json_value = lastJsonReceived.value("DATA");
                json_object = json_value.toObject();
                lastWifiNetStatus.push_back(QPair(s,json_object));
            }
        }
    }
    //emit wifiNetStatusUpdated();
    netstatusupdated = true;
    lastWifiIfStatus.clear();
    // Get status of each wireless interface
    foreach(const QString &s, wifiInterfaces)
    {
        req_string = QString::asprintf(CMD_WIFI_STATUS.c_str(), s.toStdString().c_str());
        req_object = QJsonDocument::fromJson(req_string.toUtf8()).object();
        //spy = new NMDaemonSpy(this, &NMDaemonCheckStatus::dataReady);
        spy = std::make_unique<NMDaemonSpy>(this, &NMDaemonClient::dataReady);
        if(!sendRequest(req_object))
            goto label_exit;
        if(!spy->wait(DEFAULT_READ_WAIT_TIME_MSEC))
            writeSocket.release();
        if(!lastJsonReceived.empty())
        {
            json_value = lastJsonReceived.value("RESULT");
            if(json_value.toString() == "SUCCESS")
            {
                json_value = lastJsonReceived.value("DATA");
                json_object = json_value.toObject();
                lastWifiIfStatus.push_back(QPair(s,json_object));
            }
        }
    }
label_exit:
    if(ifstatusupdated)
        emit ifStatusUpdated();
    if(netstatusupdated)
        emit wifiNetStatusUpdated();
    if(iflistupdated)
        emit wifiIfListUpdated();
    timerMonitoring->start();
}

QPair<QString,int> NMDaemonCheckStatus::getLastIfWifiNetwork(QString ifname) const
{
    QPair<QString,int> ret_status;
    QString bssid;
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    int isig=0, inoi=0;

    foreach(const auto &ifst, lastWifiIfStatus)
    {
        if(ifst.first == ifname)
        {
            bssid = ifst.second.value("BSSID").toString();
            if(bssid.isEmpty())
                continue;
            ret_status.first = ifst.second.value("SSID").toString();
            foreach(const auto &netst, lastWifiNetStatus)
            {
                if(netst.first == ifname)
                {
                    json_array = netst.second.value("NETWORKS").toArray();
                    foreach(const auto &net, json_array)
                    {
                        if(bssid == net.toObject().value("BSSID").toString())
                        {
                            isig = qAbs(net.toObject().value("SIGNAL").toInt());
                            inoi = qAbs(net.toObject().value("NOISE").toInt());
                            ret_status.second = (inoi-isig)*4;
                            return ret_status;
                        }
                    }
                }
            }
        }
    }
    return ret_status;
}

QVector<QPair<QString,int>> NMDaemonCheckStatus::getLastNetsWifiPower(QString ifname) const
{
    QVector<QPair<QString,int>> nets_power;
    QPair<QString,int> net_status;
    QJsonObject json_object;
    QJsonArray json_array;
    QJsonValue json_value;
    int isig=0, inoi=0;

    foreach(const auto &netst, lastWifiNetStatus)
    {
        if(netst.first == ifname)
        {
            json_array = netst.second.value("NETWORKS").toArray();
            foreach(const auto &net, json_array)
            {
                isig = qAbs(net.toObject().value("SIGNAL").toInt());
                inoi = qAbs(net.toObject().value("NOISE").toInt());
                net_status.first = net.toObject().value("SSID").toString();
                if(net_status.first.isEmpty())
                    net_status.first = net.toObject().value("BSSID").toString();
                net_status.second = (inoi-isig)*4;
                if(net_status.second>5)
                    nets_power.append(net_status);
            }
        }
    }
    return nets_power;
}

QString NMDaemonCheckStatus::getLastIfStatusString(QString ifname) const
{
    QString ret_string;
    QJsonArray json_arr;
    QJsonObject json_obj;
    QString str_type;
    QString str_addr;

    ret_string += "Interface Name: " + ifname + "\n";
    foreach(auto ifs, lastIfStatus)
    {
        if(ifs.toObject().value("INTERFACE NAME") == ifname)
        {
            ret_string += "Status: " + ifs.toObject().value("STATUS").toString() + "\n";
            ret_string += "Media: " + ifs.toObject().value("MEDIA").toString() + "\n";
            if(ifs.toObject().value("DHCP_ENABLED").toBool())
                ret_string += "DHCP: ENABLED\n";
            json_arr = ifs.toObject().value("ADDRESSES").toArray();
            foreach(auto addr, json_arr)
            {
                json_obj = addr.toObject();
                str_type = json_obj.value("ADDRESS TYPE").toString();
                // TODO: add support for other types of addresses
                str_addr = json_obj.value(str_type).toObject().value("IPV4 ADDRESS").toString();
                if(!str_addr.isEmpty())
                {
                    ret_string += "\nIP Address: " + str_addr + "\n";
                    ret_string += "Network mask: " + json_obj.value(str_type).toObject().value("IPV4 SUBNET MASK").toString() + "\n";
                }
            }
        }
    }
    return ret_string;
}

void NMDaemonCheckStatus::setInterval(int timeout)
{
    timerMonitoring->setInterval(timeout);
}

std::shared_ptr<QVector<QString>> NMDaemonCheckStatus::getLastIPv4AliasesFromInterface(QString ifname)
{
    std::shared_ptr<QVector<QString>> ret_aliases = std::make_shared<QVector<QString>>(0);
    QJsonObject json_object, json_obj;
    QJsonValue json_value;
    QJsonArray json_array;
    QString dev, str_value, str_type;

    if( lastIfStatus.empty() )
        return ret_aliases;

    foreach (const QJsonValue &v, lastIfStatus)
    {
        json_object = v.toObject();
        json_value = json_object.value(QString::fromStdString(JSON_PARAM_IF_NAME));
        dev = json_value.toString();
        if(dev != ifname)
            continue;
        json_array = json_object.value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
        foreach(auto addr, json_array)
        {
            json_obj = addr.toObject();
            if( !json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_PRIMARY)).isUndefined() &&
                    json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_PRIMARY)).isBool() &&
                    json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_PRIMARY)).toBool() )
                continue;
            str_type = json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_TYPE)).toString();
            str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_ADDR)).toString();
            if(!str_value.isEmpty())
            {
                str_value += " / " + json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_MASK)).toString();
                ret_aliases->push_back(str_value);
            }
        }
    }

    return ret_aliases;
}
