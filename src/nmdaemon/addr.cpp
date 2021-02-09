#include "addr.h"

addr::addr(struct ifaddrs* ifa)
{
    if(ifa->ifa_addr->sa_family == AF_LINK)
        ipType = ipaddr_type::LINK;
    else if( (ifa->ifa_flags & IFF_POINTOPOINT ) != 0 )
        ipType = ipaddr_type::PPP;
    else if(ifa->ifa_broadaddr)
        ipType = ipaddr_type::BCAST;
    else
        throw nmExcept;

    if( (ifa->ifa_flags & IFF_LOOPBACK) != 0 )
        ipType = ipaddr_type::LOOPBACK;

    if( (ifa->ifa_flags & IFF_UP) != 0 )
        isAddrUp = true;
    else
        isAddrUp = false;

    ipAddress = nullptr;
    ipMask = nullptr;
    ipData = nullptr;

    switch(ifa->ifa_addr->sa_family)
    {
        case AF_INET:
            if(ifa->ifa_addr)
                ipAddress = new address_ip4(ifa->ifa_addr);
            else
                throw nmExcept;

            if(ifa->ifa_netmask)
                ipMask = new address_ip4(ifa->ifa_netmask);
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                case ipaddr_type::LOOPBACK:
                    if(ifa->ifa_broadaddr)
                        ipData = new address_ip4(ifa->ifa_broadaddr);
                    else
                        throw nmExcept;
                    break;
                case ipaddr_type::PPP:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET) )
                        ipData = new address_ip4(ifa->ifa_dstaddr);
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET
        case AF_INET6:
            if(ifa->ifa_addr)
                ipAddress = new address_ip6(ifa->ifa_addr);
            else
                throw nmExcept;

            if(ifa->ifa_netmask)
                ipMask = new address_ip6(ifa->ifa_netmask);
            else
                throw nmExcept;

            switch(ipType)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                        ipData = new address_ip6(ifa->ifa_broadaddr);
                    else
                        throw nmExcept;
                    break;
                case ipaddr_type::PPP:
                    // We can have only IPv4 or only IPv6 ifa_dstaddr not the both
                    // In such case ifa->ifa_dstaddr->sa_family is 0
                    if( (ifa->ifa_dstaddr) && (ifa->ifa_dstaddr->sa_family==AF_INET6) )
                        ipData = new address_ip6(ifa->ifa_dstaddr);
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET6
        case AF_LINK:
            if(ifa->ifa_addr)
                ipAddress = new address_link(ifa->ifa_addr);
            else
                throw nmExcept;
            break;
        default:
            throw nmExcept;
            break;
    }
}

addr::~addr()
{
    if(ipAddress != nullptr)
        delete ipAddress;
    if(ipMask != nullptr)
        delete ipMask;
    if(ipData != nullptr)
        delete ipData;
}

// TODO: customize separator and eol strings (take them from arguments)
const std::string addr::getAddrString() const
{
    std::string retAddrStr = "";
    std::string separator = ":";
    std::string eol = "\n";
    std::string strTitle = "";

    strTitle = std::string(magic_enum::enum_name(ipType));
    retAddrStr += JSON_PARAM_ADDR_TYPE + separator + strTitle + eol;

    if(ipAddress != nullptr)
    {
        switch(ipAddress->getFamily())
        {
            case AF_INET:
                strTitle = JSON_PARAM_IPV4_ADDR;
                break;
            case AF_INET6:
                strTitle = JSON_PARAM_IPV6_ADDR;
                break;
            case AF_LINK:
                strTitle = JSON_PARAM_LINK_ADDR;
                break;
            default:
                break;
        }

        retAddrStr += strTitle + separator + ipAddress->getStrAddr() + eol;

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + ipMask->getStrAddr() + eol;
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    retAddrStr += strTitle + separator + ipData->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::PPP:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += strTitle + separator + ipMask->getStrAddr() + eol;
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_PPP_GW : JSON_PARAM_IPV6_PPP_GW;
                    retAddrStr += strTitle + separator + ipData->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::LINK:
            default:
                break;
        }
    }
    return retAddrStr;
}

const nlohmann::json addr::getAddrJson() const
{
    nlohmann::json retAddrJson;
    nlohmann::json dataJson;

    std::string strTitle = "";
    std::string strMainTitle = "";

/*
    IPv4 broadcast address JSON example:
    {
        "ADDRESS TYPE" : "BCAST",
        "BCAST" : {
            "IPV4 ADDRESS" : "192.168.211.21",
            "IPV4 SUBNET MASK" : "255.255.255.0",
            "IPV4 BROADCAST ADDRESS" : "192.168.211.255"
        }
    }
*/
    strMainTitle = std::string(magic_enum::enum_name(ipType));
    retAddrJson[JSON_PARAM_ADDR_TYPE] = strMainTitle;

    if(ipAddress != nullptr)
    {
        switch(ipAddress->getFamily())
        {
            case AF_INET:
                strTitle = JSON_PARAM_IPV4_ADDR;
                break;
            case AF_INET6:
                strTitle = JSON_PARAM_IPV6_ADDR;
                break;
            case AF_LINK:
                strTitle = JSON_PARAM_LINK_ADDR;
                break;
            default:
                break;
        }

        dataJson[strTitle] = ipAddress->getStrAddr();

        switch(ipType)
        {
            case ipaddr_type::BCAST:
            case ipaddr_type::LOOPBACK:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = ipMask->getStrAddr();
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    dataJson[strTitle] = ipData->getStrAddr();
                }
                break;
            case ipaddr_type::PPP:
                if(ipMask != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    dataJson[strTitle] = ipMask->getStrAddr();
                }
                if(ipData != nullptr)
                {
                    strTitle = (ipAddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_PPP_GW : JSON_PARAM_IPV6_PPP_GW;
                    dataJson[strTitle] = ipData->getStrAddr();
                }
                break;
            case ipaddr_type::LINK:
            // TODO: show link data (speed, status etc.)
            default:
                break;
        }

        retAddrJson[strMainTitle] = dataJson;
    }

    return retAddrJson;
}

bool addr::isUp() const
{
    return isAddrUp;
}

short addr::getFamily() const
{
    return ipAddress->getFamily();
}
