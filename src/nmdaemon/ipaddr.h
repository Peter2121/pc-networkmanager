#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include "json.hpp"
#include "nmjsonconst.h"
#include "address.h"

enum class ipaddr_type
{
    BCAST, PPP
};

class ipaddr
{
protected:
    bool mmanage;
    ipaddr_type iptype;
    address* ipaddress;
    address* ipmask;
    address* ipdata;    // network broadcast or ppp gateway
public:
//    ipaddr(address*, address*, address*, ipaddr_type);
    ipaddr(struct ifaddrs*);
    ~ipaddr();
    std::string getIpAddrString();
    nlohmann::json getIpAddrJson();
};

#endif // IPADDR_H
