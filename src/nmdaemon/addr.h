#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include "json.hpp"
#include "magic_enum.hpp"
#include "nmjsonconst.h"
#include "address_base.h"
#include "address_ip4.h"
#include "address_ip6.h"
#include "address_link.h"

enum class ipaddr_type
{
    BCAST, PPP, LINK
};

class addr
{
protected:
    ipaddr_type iptype;
    address_base* ipaddress;
    address_base* ipmask;    // subnet mask or nothing
    address_base* ipdata;    // network broadcast or ppp gateway or nothing
public:
    addr(struct ifaddrs*);
    ~addr();
    std::string getIpAddrString();
    nlohmann::json getIpAddrJson();
};

#endif // IPADDR_H
