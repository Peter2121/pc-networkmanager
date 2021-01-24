#include "addr.h"

addr::addr(struct ifaddrs* ifa)
{
    if(ifa->ifa_addr->sa_family == AF_LINK)
        iptype = ipaddr_type::LINK;
    else if(ifa->ifa_dstaddr)
        iptype = ipaddr_type::PPP;
    else if(ifa->ifa_broadaddr)
        iptype = ipaddr_type::BCAST;
/*  else
        // TODO: Raise exception
*/
    ipaddress = nullptr;
    ipmask = nullptr;
    ipdata = nullptr;
    switch(ifa->ifa_addr->sa_family)
    {
        case AF_INET:
            if(ifa->ifa_addr)
                ipaddress = new address_ip4(ifa->ifa_addr);
        /*  else
                // TODO: Raise exception
        */
            if(ifa->ifa_netmask)
                ipmask = new address_ip4(ifa->ifa_netmask);
        /*  else
                // TODO: Raise exception
        */
            switch(iptype)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                        ipdata = new address_ip4(ifa->ifa_broadaddr);
                /*  else
                        // TODO: Raise exception
                */
                    break;
                case ipaddr_type::PPP:
                    if(ifa->ifa_dstaddr)
                        ipdata = new address_ip4(ifa->ifa_dstaddr);
                /*  else
                        // TODO: Raise exception
                */
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET
        case AF_INET6:
            if(ifa->ifa_addr)
                ipaddress = new address_ip6(ifa->ifa_addr);
        /*  else
                // TODO: Raise exception
        */
            if(ifa->ifa_netmask)
                ipmask = new address_ip6(ifa->ifa_netmask);
        /*  else
                // TODO: Raise exception
        */
            switch(iptype)
            {
                case ipaddr_type::BCAST:
                    if(ifa->ifa_broadaddr)
                        ipdata = new address_ip6(ifa->ifa_broadaddr);
                /*  else
                        // TODO: Raise exception
                */
                    break;
                case ipaddr_type::PPP:
                    if(ifa->ifa_dstaddr)
                        ipdata = new address_ip6(ifa->ifa_dstaddr);
                /*  else
                        // TODO: Raise exception
                */
                    break;
                default:
                    break;
            }
            break;  // end of case AF_INET6
        case AF_LINK:
            if(ifa->ifa_addr)
                ipaddress = new address_link(ifa->ifa_addr);
        /*  else
                // TODO: Raise exception
        */
            break;
        default:
            // TODO: Raise exception
            break;
    }
}

addr::~addr()
{
    if(ipaddress != nullptr)
        delete ipaddress;
    if(ipmask != nullptr)
        delete ipmask;
    if(ipdata != nullptr)
        delete ipdata;
}

// TODO: customize separtor and eol strings (take them from arguments)
std::string addr::getIpAddrString()
{
    std::string retAddrStr = "";
    std::string separator = ":";
    std::string eol = "\n";
    std::string title = "";

    retAddrStr += JSON_PARAM_ADDR_TYPE + separator + std::string(magic_enum::enum_name(iptype)) + eol;

    if(ipaddress != nullptr)
    {
        switch(ipaddress->getFamily())
        {
            case AF_INET:
                title = JSON_PARAM_IPV4_ADDR;
                break;
            case AF_INET6:
                title = JSON_PARAM_IPV6_ADDR;
                break;
            case AF_LINK:
                title = JSON_PARAM_LINK_ADDR;
                break;
            default:
                break;
        }

        retAddrStr += title + separator + ipaddress->getStrAddr() + eol;

        switch(iptype)
        {
            case ipaddr_type::BCAST:
                if(ipmask != nullptr)
                {
                    title = (ipaddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += title + separator + ipmask->getStrAddr() + eol;
                }
                if(ipdata != nullptr)
                {
                    title = (ipaddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_BCAST : JSON_PARAM_IPV6_BCAST;
                    retAddrStr += title + separator + ipdata->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::PPP:
                if(ipmask != nullptr)
                {
                    title = (ipaddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_MASK : JSON_PARAM_IPV6_MASK;
                    retAddrStr += title + separator + ipmask->getStrAddr() + eol;
                }
                if(ipdata != nullptr)
                {
                    title = (ipaddress->getFamily() == AF_INET) ? JSON_PARAM_IPV4_PPP_GW : JSON_PARAM_IPV6_PPP_GW;
                    retAddrStr += title + separator + ipdata->getStrAddr() + eol;
                }
                break;
            case ipaddr_type::LINK:
            default:
                break;
        }
    }
    return retAddrStr;
}

nlohmann::json addr::getIpAddrJson()
{
}
