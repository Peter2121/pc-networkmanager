#include "nmdaemoncheckstatus.h"

NMDaemonCheckStatus::NMDaemonCheckStatus(int refresh_time, QString socket_path) : NMDaemonClient(socket_path, true),
                                                              refreshTime(refresh_time), isMonitoring(false)
{
    spy = nullptr;
    timerMonitoring = new QTimer(this);
    timerMonitoring->setInterval(refreshTime);
    connect(timerMonitoring, &QTimer::timeout, this, &NMDaemonCheckStatus::UpdateStatus);
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
        StopMonitoring();
    delete timerMonitoring;
}

void NMDaemonCheckStatus::StartMonitoring()
{
    if(isMonitoring)
        return;
    if(!DoConnect())
        return;
    isMonitoring = true;
    UpdateStatus();
    timerMonitoring->start();
}

void NMDaemonCheckStatus::SlotStartMonitoring()
{
    StartMonitoring();
}

void NMDaemonCheckStatus::StopMonitoring()
{
    if(isMonitoring)
        timerMonitoring->stop();
    isMonitoring = false;
}

bool NMDaemonCheckStatus::UpdateStatusInterfaces()
{
    QJsonValue json_value;
    QJsonArray json_array;
    QJsonObject json_obj;
    timerMonitoring->stop();
    lastIfStatus = QJsonArray();
    QString req_string = CMD_GET_SYS_IF_STATUS;
    QJsonObject ret_object = AsyncSendCommandGetResult(req_string, "UpdateStatusInterfaces");
    if(ret_object.empty())
    {
        timerMonitoring->start();
        return false;
    }
    json_value = ret_object.value(QString::fromStdString(JSON_PARAM_DATA));
    json_obj = json_value.toObject();
    json_value = json_obj.value(QString::fromStdString(JSON_PARAM_INTERFACES));
    json_array = json_value.toArray();
    lastIfStatus = json_array;
    emit ifStatusUpdated();
    timerMonitoring->start();
    return true;
}

bool NMDaemonCheckStatus::UpdateStatusWifi()
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    QString req_string;
    QJsonObject ret_object;
    QString if_name;
    bool updated = false;
    timerMonitoring->stop();
    req_string = CMD_GET_WPA_IF_LIST;
    wifiInterfaces.clear();
    ret_object = AsyncSendCommandGetResult(req_string, "UpdateStatusWifi-iflist");
    if(ret_object.empty())
    {
        timerMonitoring->start();
        return false;
    }
    json_value = lastJsonReceived.value("DATA");
    json_object = json_value.toObject();
    json_value = json_object.value("INTERFACES");
    json_array = json_value.toArray();
    foreach (const QJsonValue &v, json_array)
    {
        wifiInterfaces.push_back(v.toString());
    }
    if(wifiInterfaces.empty())
    {
        timerMonitoring->start();
        return false;
    }
    lastWifiNetStatus.clear();
    foreach(const QString &s, wifiInterfaces)
    {
        req_string = QString::asprintf(CMD_WIFI_SCAN.c_str(), s.toStdString().c_str());
        ret_object = AsyncSendCommandGetResult(req_string, "UpdateStatusWifi-scan");
        if(ret_object.empty())
            continue;
        json_value = lastJsonReceived.value("DATA");
        json_object = json_value.toObject();
        lastWifiNetStatus.push_back(QPair<QString,QJsonObject>(s, json_object));
        updated = true;
    }
    lastWifiIfStatus.clear();
    foreach(const QString &s, wifiInterfaces)
    {
        req_string = QString::asprintf(CMD_WIFI_STATUS.c_str(), s.toStdString().c_str());
        ret_object = AsyncSendCommandGetResult(req_string, "UpdateStatusWifi-status");
        if(ret_object.empty())
            continue;
        json_value = lastJsonReceived.value("DATA");
        json_object = json_value.toObject();
        lastWifiIfStatus.push_back(QPair<QString,QJsonObject>(s,json_object));
        updated = true;
    }
    timerMonitoring->start();
    if(updated)
        emit wifiNetStatusUpdated();
    return updated;
}

void NMDaemonCheckStatus::UpdateStatus()
{
    UpdateStatusInterfaces();
    UpdateStatusJails();
    UpdateStatusWifi();
}

QPair<QString,int> NMDaemonCheckStatus::GetLastIfWifiNetwork(QString ifname) const
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

QVector<QPair<QString,int>> NMDaemonCheckStatus::GetLastNetsWifiPower(QString ifname) const
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
                //if(net_status.second>5) {
                nets_power.append(net_status);
                //}
            }
        }
    }
    return nets_power;
}

QVector<WifiNetworkDetails> NMDaemonCheckStatus::GetLastNetsWifiDetails(QString ifname) const
{
    QVector<WifiNetworkDetails> nets_details;
    WifiNetworkDetails details;
    QJsonObject json_object;
    QJsonArray json_array;
    QJsonValue json_value;
    int isig=0, inoi=0;
    QString caps, ies;

    foreach(const auto &netst, lastWifiNetStatus)
    {
        if(netst.first == ifname)
        {
            json_array = netst.second.value("NETWORKS").toArray();
            foreach(const auto &net, json_array)
            {
                isig = qAbs(net.toObject().value("SIGNAL").toInt());
                inoi = qAbs(net.toObject().value("NOISE").toInt());
                details.SSID = net.toObject().value("SSID").toString();
                if(details.SSID.isEmpty())
                    details.SSID = net.toObject().value("BSSID").toString();
                details.Power = (inoi-isig)*4;
                if(details.Power>5)
                {
                    caps = net.toObject().value("CAPS").toString();
                    ies = net.toObject().value("IES").toString();
                    if( (!caps.isEmpty()) && (!ies.isEmpty()) )
                    {
                        details.Security = GetWifiNetworkSecurity(caps, ies);
                    }
                    nets_details.push_back(details);
                }
            }
        }
    }

    return nets_details;
}

QString NMDaemonCheckStatus::GetLastIfStatusString(QString ifname) const
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

void NMDaemonCheckStatus::SetInterval(int timeout)
{
    timerMonitoring->setInterval(timeout);
}

std::shared_ptr<QVector<QString>> NMDaemonCheckStatus::GetLastIPv4AliasesFromInterface(QString ifname)
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
        foreach(const auto &addr, json_array)
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

std::unique_ptr<QVector<QString>> NMDaemonCheckStatus::GetLastJailsIPv4()
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    std::unique_ptr<QVector<QString>> ret_addr4 = std::make_unique<QVector<QString>>(0);
    foreach(const auto &jail, lastJailStatus)
    {
        json_object = jail.toObject();
        json_array = json_object.value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
        foreach (const auto &addr, json_array) {
            if( const auto ipv4addr=addr.toObject().value(QString::fromStdString(JSON_PARAM_IPV4_ADDR)).toString(); !ipv4addr.isEmpty() )
            {
                ret_addr4->push_back(ipv4addr);
            }
        }
    }
    return ret_addr4;
}

std::unique_ptr<QVector<QString>> NMDaemonCheckStatus::GetLastJailsIPv6()
{
    QJsonObject json_object;
    QJsonValue json_value;
    QJsonArray json_array;
    std::unique_ptr<QVector<QString>> ret_addr6 = std::make_unique<QVector<QString>>(0);
    foreach(const auto &jail, lastJailStatus)
    {
        json_object = jail.toObject();
        json_array = json_object.value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
        foreach (const auto &addr, json_array) {
            if( const auto ipv6addr=addr.toObject().value(QString::fromStdString(JSON_PARAM_IPV6_ADDR)).toString(); !ipv6addr.isEmpty() )
            {
                ret_addr6->push_back(ipv6addr);
            }
        }
    }
    return ret_addr6;
}

bool NMDaemonCheckStatus::UpdateStatusJails()
{
    QJsonValue json_value;
    QJsonArray json_array;
    if(!DoConnect())
        return false;
    timerMonitoring->stop();
    lastJailStatus = QJsonArray();
    QString req_string = CMD_GET_SYS_JAIL_LIST;
    QJsonObject ret_object = AsyncSendCommandGetResult(req_string, "UpdateStatusJails");
    if(ret_object.empty())
    {
        timerMonitoring->start();
        return false;
    }
    json_value = ret_object.value(QString::fromStdString(JSON_PARAM_DATA)).toObject().value(QString::fromStdString(JSON_PARAM_JAILS));
    json_array = json_value.toArray();
    lastJailStatus = json_array;
    emit jailStatusUpdated();
    timerMonitoring->start();
    return true;
}

