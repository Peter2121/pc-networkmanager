#include "ethernetconfigdata.h"

EthernetConfigData::EthernetConfigData(ethernetconfig* ethc, NMDaemonCheckStatus* nmd)
{
    isInitialized = false;
    ipv4Aliases = std::make_shared<QVector<QString>>(0);
    isMacDefault = true;
    isIpv4Static = true;
    isIpv6Static = false;

    QJsonValue json_value;
    QJsonObject json_object, json_obj;
    QJsonArray json_array;
    QString str_value,str_type,dev;
    bool is_up=false, is_running=false;

    try
    {
        ifName = ethc->getDeviceName();
        ipv4Address = nmd->getIPv4Address(ifName);
        ipv4Aliases = nmd->getLastIPv4AliasesFromInterface(ifName);
        QJsonArray last_if_status = nmd->getLastIfStatus();

        if(!last_if_status.empty())
        {
            foreach (const QJsonValue &v, last_if_status)
            {
                json_object = v.toObject();
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_IF_NAME));
                dev = json_value.toString();
                if(dev != ifName)
                    continue;
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_MEDIA));
                str_value = json_value.toString();
//                if(!str_value.contains("Ethernet"))
//                     return;
                str_value = json_object.value(QString::fromStdString(JSON_PARAM_STATUS)).toString();
                ifStatus = str_value;
                str_value = json_object.value(QString::fromStdString(JSON_PARAM_MEDIA)).toString();
                mediaStatus = str_value;
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_DHCP_ENABLED));
                if(json_value.isBool())
                    isIpv4Static = !json_value.toBool();
                else
                    isIpv4Static = true;
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_UP));
                if(json_value.isBool())
                    is_up = json_value.toBool();
                json_value = json_object.value(QString::fromStdString(JSON_PARAM_RUNNING));
                if(json_value.isBool())
                    is_running = json_value.toBool();
                isUpAndRunning = is_up && is_running;
                json_array = json_object.value(QString::fromStdString(JSON_PARAM_ADDRESSES)).toArray();
                ipv4Aliases->clear();
                foreach(auto addr, json_array)
                {
                    json_obj = addr.toObject();
                    str_type = json_obj.value(QString::fromStdString(JSON_PARAM_ADDR_TYPE)).toString();
                    str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_ADDR)).toString();
                    if(!str_value.isEmpty())
                    {
                        if(str_value == ipv4Address)
                        {
                            ipv4Mask = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_MASK)).toString();
                        }
                        else
                        {
                            str_value += " / " + json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV4_MASK)).toString();
                            ipv4Aliases->push_back(str_value);
                        }
                    }
                    str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_LINK_ADDR)).toString();
                    if(!str_value.isEmpty())
                    {
                        macAddress = str_value;
                    }
                    str_value = json_obj.value(str_type).toObject().value(QString::fromStdString(JSON_PARAM_IPV6_ADDR)).toString();
                    if(!str_value.isEmpty())
                    {
                        ipv6Address = str_value;
                    }
                }
            }
            isInitialized = true;
         }
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(ethc, QString("Cannot parse NMDaemon data: ")+QString(e.what()), QString("EthernetConfigData"));
    }
}

EthernetConfigData::EthernetConfigData(ethernetconfig* ethc)
{
    isInitialized = false;

    isMacDefault = true;
    isIpv6Static = false;

    try
    {
        ifName = ethc->getDeviceName();
        ipv4Address = ethc->lineIP->text();
        ipv4Mask = ethc->lineNetmask->text();
        ipv4Aliases = ethc->getIPv4AliasesFromUi();
        macAddress = ethc->textMac->text();
        ipv6Address = ethc->textIPv6->text();
        mediaStatus = ethc->textMedia->text();
        ifStatus = ethc->textStatus->text();
        isUpAndRunning = ethc->checkDisableNetwork->isChecked();
        isIpv4Static = ethc->groupStatic->isChecked();

        isInitialized = true;
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(ethc, QString("Cannot parse EthernetConfig data: ")+QString(e.what()), QString("EthernetConfigData"));
    }
}

bool EthernetConfigData::operator ==(const EthernetConfigData &ecd) const
{
    if( (!isInitialized) && (!ecd.isInitialized) )
        return true;
    if( (!isInitialized) || (!ecd.isInitialized) )
        return false;

    // QString ifName;          IGNORED!!!
    // bool isUpAndRunning;     IGNORED!!!
    // QString mediaStatus;     IGNORED!!!
    // QString ifStatus;        IGNORED!!!
    if( isIpv4Static != ecd.isIpv4Static)
        return false;
    if( isIpv6Static != ecd.isIpv6Static)
        return false;
    if( isMacDefault != ecd.isMacDefault)
        return false;
    if( macAddress != ecd.macAddress)
        return false;
    if( ipv6Address != ecd.ipv6Address)
        return false;
    if( ipv4Aliases->size() != ecd.ipv4Aliases->size() )
        return false;
    if( (ipv4Aliases->size()==0) && (ecd.ipv4Aliases->size()==0) )
    {
    if( ipv4Address != ecd.ipv4Address)
        return false;
    if( ipv4Mask != ecd.ipv4Mask)
        return false;
    return true;
    }
    std::unique_ptr<QVector<QString>> my_aliases = std::make_unique<QVector<QString>>(*ipv4Aliases.get());
    my_aliases->push_back(ipv4Address + " / " + ipv4Mask);
    std::sort(my_aliases->begin(), my_aliases->end());
    std::unique_ptr<QVector<QString>> other_aliases = std::make_unique<QVector<QString>>(*ecd.ipv4Aliases.get());
    other_aliases->push_back(ecd.ipv4Address + " / " + ecd.ipv4Mask);
    std::sort(other_aliases->begin(), other_aliases->end());
    return my_aliases==other_aliases;
}

bool EthernetConfigData::operator !=(const EthernetConfigData &ecd) const
{
    return !(*this==ecd);
}
