#ifndef ETHERNETCONFIGDATA_H
#define ETHERNETCONFIGDATA_H

#include <algorithm>
#include <QString>
#include <QMessageBox>
#include "../NMDaemonClient/nmdaemoncheckstatus.h"
//#include "ethernetconfig.h"

class ethernetconfig;

class EthernetConfigData
{
public:
    EthernetConfigData(ethernetconfig*, NMDaemonCheckStatus*);
    EthernetConfigData(ethernetconfig*);

    bool operator ==(const EthernetConfigData &) const;
    bool operator !=(const EthernetConfigData &) const;

public:
    bool isInitialized;
    QString ifName;
    QString ipv4Address;
    QString ipv4Mask;
    QString ipv4Gateway;
    QString macAddress;
    bool isUpAndRunning;
    bool isIpv4Static;
    std::shared_ptr<QVector<QString>> ipv4Aliases;
    bool isMacDefault;
    bool isIpv6Static;
    QString ipv6Address;
    QString mediaStatus;
    QString ifStatus;
};

#endif // ETHERNETCONFIGDATA_H
