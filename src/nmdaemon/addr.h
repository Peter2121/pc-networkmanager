#ifndef IPADDR_H
#define IPADDR_H

#include <ifaddrs.h>
#include <net/if.h>
#include "json.hpp"
#include "magic_enum.hpp"
#include "nmjsonconst.h"
#include "address_base.h"
#include "address_ip4.h"
#include "address_ip6.h"
#include "address_link.h"
#include "nmexception.h"

enum class ipaddr_type
{
    BCAST, PPP, LINK, LOOPBACK
};

class addr
{
protected:
    ipaddr_type ipType;
    address_base* ipAddress;
    address_base* ipMask;    // subnet mask or nothing
    address_base* ipData;    // network broadcast or ppp gateway or nothing
    bool isAddrUp;
    nmexception nmExcept;
public:
    addr(struct ifaddrs*);
    addr(address_base* addr, address_base* mask, address_base* data=nullptr, ipaddr_type type=ipaddr_type::BCAST, bool up=false);
    ~addr();
    const address_base* getAddrAB() const;
    const address_base* getMaskAB() const;
    const address_base* getDataAB() const;
    const std::string getAddrString() const;
    const nlohmann::json getAddrJson() const;
    bool isUp() const;
    short getFamily() const;
};

#endif // IPADDR_H
