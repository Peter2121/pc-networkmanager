#include "if_worker.h"

if_worker::if_worker()
{ }

if_worker::~if_worker()
{ }

nmscope if_worker::getScope()
{
    return nmscope::INTERFACE;
}

json if_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case nmcmd::IP_ADDR_SET :
            return execCmdIpAddrSet(pcmd);
        case nmcmd::IP4_DHCP_ENABLE :
        case nmcmd::IP6_DHCP_ENABLE :
        case nmcmd::IP4_ALIAS_ADD :
        case nmcmd::IP6_ALIAS_ADD :
        case nmcmd::IP4_ALIAS_REMOVE :
        case nmcmd::IP6_ALIAS_REMOVE :
        case nmcmd::MTU_GET :
        case nmcmd::MTU_SET :
        case nmcmd::MAC_ADDR_GET :
        case nmcmd::MAC_ADDR_SET :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool if_worker::isValidCmd(nmcommand_data* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;
    int cmd_size = sizeof(cmds) / sizeof(cmds[0]);
    for(int i=0; i<cmd_size; i++)
    {
        if(pcmd->getCommand().cmd == cmds[i].cmd)
            return true;
    }
    return false;
}

/*
static bool set_address_and_mask(struct in_aliasreq *ifra, host_t *addr, u_int8_t netmask)
{
    host_t *mask;
    memcpy(&ifra->ifra_addr, addr->get_sockaddr(addr), *addr->get_sockaddr_len(addr));
    // set the same address as destination address
    memcpy(&ifra->ifra_dstaddr, addr->get_sockaddr(addr), *addr->get_sockaddr_len(addr));
    mask = host_create_netmask(addr->get_family(addr), netmask);
    if (!mask)
    {
        DBG1(DBG_LIB, "invalid netmask: %d", netmask);
        return FALSE;
    }
    memcpy(&ifra->ifra_mask, mask->get_sockaddr(mask), *mask->get_sockaddr_len(mask));
    mask->destroy(mask);
    return TRUE;
}

//  Set the address using the more flexible SIOCAIFADDR/SIOCDIFADDR commands on FreeBSD 10 an newer.
static bool set_address_impl(private_tun_device_t *this, host_t *addr, u_int8_t netmask)
{
    struct in_aliasreq ifra;
    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, this->if_name, IFNAMSIZ);
    if (this->address)
    {	// remove the existing address first
        if (!set_address_and_mask(&ifra, this->address, this->netmask))
        {
            return FALSE;
        }
        if (ioctl(this->sock, SIOCDIFADDR, &ifra) < 0)
        {
            DBG1(DBG_LIB, "failed to remove existing address on %s: %s",
                 this->if_name, strerror(errno));
            return FALSE;
        }
    }
    if (!set_address_and_mask(&ifra, addr, netmask))
    {
        return FALSE;
    }
    if (ioctl(this->sock, SIOCAIFADDR, &ifra) < 0)
    {
        DBG1(DBG_LIB, "failed to add address on %s: %s",
             this->if_name, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

struct ifaliasreq {
    char    ifra_name[IFNAMSIZ];   // if name,	e.g. "en0"
    struct  sockaddr	ifra_addr;
    struct  sockaddr	ifra_broadaddr;
    struct  sockaddr	ifra_mask;
}


    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0)
    {
        perror ("socket error");
        exit(1);
    }
    result = ioctl(s, SIOCGIFADDR, &ifr);
    if(result < 0)
    {
        perror("ioctl ifr error");
        exit(1);
    }

*/

// The return value must be deallocated in calling function
address_base* if_worker::getMainIfAddr(short family) // family: AF_INET / AF_INET6
{
    struct ifreq ifr;
    address_base* paddr=nullptr;
//    address_base* pmask=nullptr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strlcpy(ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = family;

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCGIFADDR, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get current interface " << ifName << " address";
        sock.close();
        return paddr;
    }

    try {
        switch(family)
        {
            case AF_INET:
                paddr = new address_ip4(&ifr.ifr_addr);
                break;
            case AF_INET6:
                paddr = new address_ip6(&ifr.ifr_addr);
                break;
            default:
                break;
        }
    } catch (std::exception& e) {
        sock.close();
        LOG_S(ERROR) << "Cannot get current interface " << ifName << "address";
        return nullptr;
    }

    LOG_S(INFO) << "Got current IP address: " + paddr->getStrAddr() + " for interface " << ifName;
/*
    strlcpy(ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = family;
    if (ioctl(sock.handle(), SIOCGIFNETMASK, (caddr_t)&ifr) < 0)
    {
        LOG_S(ERROR) << "Cannot get current interface " << ifName << " netmask";
        sock.close();
        return paddr;
    }
    sock.close();

    try {
        switch(family)
        {
            case AF_INET:
                pmask = new address_ip4(&ifr.ifr_addr);
                break;
            case AF_INET6:
                pmask = new address_ip6(&ifr.ifr_addr);
                break;
            default:
                break;
        }
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Cannot get current interface " << ifName << "address";
        return nullptr;
    }
*/
    sock.close();
    return paddr;
}

bool if_worker::removeIfAddr(address_base* paddrb)
{
    struct ifaliasreq ifra;
    memset(&ifra, 0, sizeof(struct ifaliasreq));

    strlcpy(ifra.ifra_name, ifName.c_str(), sizeof(ifra.ifra_name));
    memcpy(&ifra.ifra_addr, paddrb->getSockAddr(), sizeof(struct sockaddr));

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCDIFADDR, (caddr_t)&ifra) < 0)
    {
        LOG_S(ERROR) << "Cannot remove current address from interface " << ifName;
        sock.close();
        return false;
    }

    sock.close();
    return true;
}

bool if_worker::addIfAddr(addr* paddr)
{
    struct ifaliasreq ifra;
    memset(&ifra, 0, sizeof(struct ifaliasreq));

/*
    sa_in = (struct sockaddr_in *) &(ifar_new_alias.ifra_addr);
    sa_in->sin_family = AF_INET;
    sa_in->sin_len = sizeof(struct sockaddr_in);
    sa_in->sin_addr.s_addr = ia_newip.s_addr;

    sa_in = (struct sockaddr_in *) &(ifar_new_alias.ifra_broadaddr);
    sa_in->sin_family = AF_INET;
    sa_in->sin_len = sizeof(struct sockaddr_in);
    sa_in->sin_addr.s_addr = ia_nebcast.s_addr;

    sa_in = (struct sockaddr_in *) &(ifar_new_alias.ifra_mask);
    sa_in->sin_family = AF_INET;
    sa_in->sin_len = sizeof(struct sockaddr_in);
    sa_in->sin_addr.s_addr = ia_newmask.s_addr;

    strcpy(ifar_new_alias.ifra_name, ifname);
*/

    strlcpy(ifra.ifra_name, ifName.c_str(), sizeof(ifra.ifra_name));
    const address_base* ab = paddr->getAddrAB();
    const struct sockaddr* sa = ab->getSockAddr();
    memcpy(&ifra.ifra_addr, &sa, sizeof(struct sockaddr));
    ab = paddr->getMaskAB();
    sa = ab->getSockAddr();
    memcpy(&ifra.ifra_mask, &sa, sizeof(struct sockaddr));

    sockpp::socket sock = sockpp::socket::create(AF_INET, SOCK_DGRAM);
    if (ioctl(sock.handle(), SIOCAIFADDR, (caddr_t)&ifra) < 0)
    {
        LOG_S(ERROR) << "Cannot add address:\n" << paddr->getAddrString() << "to interface " << ifName;
        sock.close();
        return false;
    }

    sock.close();
    return true;
}

json if_worker::execCmdIpAddrSet(nmcommand_data* pcmd)
{
    std::string str_ifname = "";
    std::string str_ifaddr = "";
    std::string str_ifmask = "";
    addr* new_if_addr = nullptr;
//    addr* cur_if_addr = nullptr;
    address_base* ifaddr = nullptr;
    address_base* ifmask = nullptr;
    address_base* cur_if_addr = nullptr;
    short ip_family = 0;

    json cmd_json = {};
    json cmd_json_data = {};
    try {
        cmd_json = pcmd->getJsonData();
        str_ifname = cmd_json[JSON_PARAM_IF_NAME];
        cmd_json_data = cmd_json[JSON_PARAM_DATA];
        ifName = str_ifname;

        if(cmd_json_data.contains(JSON_PARAM_IPV4_ADDR)) {
            str_ifaddr = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IPV4_ADDR];
            str_ifmask = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IPV4_MASK];
            ip_family = AF_INET;
        }
        else if(cmd_json_data.contains(JSON_PARAM_IPV6_ADDR))
        {
            str_ifaddr = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IPV6_ADDR];
            str_ifmask = cmd_json[JSON_PARAM_DATA][JSON_PARAM_IPV6_MASK];
            ip_family = AF_INET6;
        }
    } catch (std::exception& e) {
        LOG_S(ERROR) << "Exception in execCmdIpAddrSet - cannot get interface parameters";
        return JSON_RESULT_ERR;
    }

    if(str_ifaddr.empty() || str_ifmask.empty()) {
        LOG_S(ERROR) << "execCmdIpAddrSet - Cannot get interface parameters from JSON";
        return JSON_RESULT_ERR;
    }

    switch(ip_family) {
        case AF_INET:
            try {
                ifaddr = new address_ip4(str_ifaddr);
                ifmask = new address_ip4(str_ifmask);
            } catch (std::exception& e) {
                LOG_S(ERROR) << "execCmdIpAddrSet - Cannot create ip4 address from JSON parameters";
                return JSON_RESULT_ERR;
            }
            break;
        case AF_INET6:
            try {
                ifaddr = new address_ip6(str_ifaddr);
                ifmask = new address_ip6(str_ifmask);
            } catch (std::exception& e) {
                LOG_S(ERROR) << "execCmdIpAddrSet - Cannot create ip6 address from JSON parameters";
                return JSON_RESULT_ERR;
            }
            break;
        default:
            LOG_S(ERROR) << "execCmdIpAddrSet - Cannot get address type from JSON";
            return JSON_RESULT_ERR;
    }

    new_if_addr = new addr(ifaddr, ifmask);
    cur_if_addr = getMainIfAddr(ip_family);
    if(cur_if_addr==nullptr) {
        LOG_S(ERROR) << "execCmdIpAddrSet - Cannot get current IP address of interface";
        return JSON_RESULT_ERR;
    }
    if(!removeIfAddr(cur_if_addr)) {
        delete cur_if_addr;
        delete new_if_addr;
        LOG_S(ERROR) << "execCmdIpAddrSet - Cannot remove current IP address from interface";
        return JSON_RESULT_ERR;
    }

    delete cur_if_addr;
    if(!addIfAddr(new_if_addr)) {
        delete new_if_addr;
        LOG_S(ERROR) << "execCmdIpAddrSet - Cannot add new IP address to interface";
        return JSON_RESULT_ERR;
    }

    delete new_if_addr;
    return JSON_RESULT_SUCCESS;
}
